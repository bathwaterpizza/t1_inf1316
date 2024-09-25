#include "cfg.h"
#include "types.h"
#include "util.h"
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Whether the kernel is running and reading the interrupt controller pipe
static bool kernel_running;
// Queue for apps waiting on device D1
static queue_t *D1_app_queue;
// Queue for apps waiting on device D2
static queue_t *D2_app_queue;
// PID of the intersim process
static pid_t intersim_pid;
// Array of app info structs
static proc_info_t apps[APP_AMOUNT];
// Shared memory segment between apps and kernel
static int *shm;
// Represents the index of the next app to (possibly) continue execution, upon
// receiving a timeslice interrupt. Its value cycles through the apps[] array
static int schedule_next_app_index = 1;

// Returns the app_id of the child app with the given pid,
// or -1 if none found
static int pid_to_appid(int pid) {
  for (int i = 0; i < APP_AMOUNT; i++) {
    if (apps[i].app_pid == pid) {
      return apps[i].app_id;
    }
  }

  return -1;
}

// Returns whether all apps have finished executing
static bool all_apps_finished(void) {
  for (int i = 0; i < APP_AMOUNT; i++) {
    if (apps[i].state != FINISHED) {
      return false;
    }
  }

  return true;
}

// Returns how many apps have finished so far
static int get_amount_finished(void) {
  int count = 0;

  for (int i = 0; i < APP_AMOUNT; i++) {
    if (apps[i].state == FINISHED) {
      count++;
    }
  }

  return count;
}

// Returns the appid of the current running app
static int get_running_appid(void) {
  for (int i = 0; i < APP_AMOUNT; i++) {
    if (apps[i].state == RUNNING) {
      return apps[i].app_id;
    }
  }

  return -1;
}

// Updates the stats of an app according to the syscall type
static void update_app_stats(syscall_t call, int app_index) {
  switch (call) {
  case SYSCALL_D1_R:
    apps[app_index].D1_access_count++;
    apps[app_index].read_count++;
    break;
  case SYSCALL_D1_W:
    apps[app_index].D1_access_count++;
    apps[app_index].write_count++;
    break;
  case SYSCALL_D1_X:
    apps[app_index].D1_access_count++;
    apps[app_index].exec_count++;
    break;
  case SYSCALL_D2_R:
    apps[app_index].D2_access_count++;
    apps[app_index].read_count++;
    break;
  case SYSCALL_D2_W:
    apps[app_index].D2_access_count++;
    apps[app_index].write_count++;
    break;
  case SYSCALL_D2_X:
    apps[app_index].D2_access_count++;
    apps[app_index].exec_count++;
    break;
  default:
    fprintf(stderr, "update_app_stats error\n");
    exit(7);
  }
}

// Called when kernelsim receives a syscall from one of the apps
static void handle_app_syscall(int signum, siginfo_t *info, void *context) {
  // Block signals
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);
  sigaddset(&mask, SIGCHLD);
  if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
    fprintf(stderr, "Signal blocking error\n");
    exit(9);
  }

  int app_id = pid_to_appid(info->si_pid);
  int app_index = app_id - 1;
  syscall_t call = get_app_syscall(shm, app_id);

  if (apps[app_index].state != RUNNING) {
    write_log("WARN: Handling syscall for non-running app %d", app_id);
  }
  assert(call != SYSCALL_NONE);

  // send signal to save app context, then it'll sigstop itself
  kill(info->si_pid, SIGUSR1);
  apps[app_index].state = BLOCKED;
  write_log("App %d blocked for syscall %s", app_id, SYSCALL_STR[call]);

  update_app_stats(call, app_index);

  if (call >= SYSCALL_D1_R && call <= SYSCALL_D1_X) {
    enqueue(D1_app_queue, app_id);
  } else {
    enqueue(D2_app_queue, app_id);
  }

  // Unblock signals
  if (sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0) {
    fprintf(stderr, "Signal blocking error\n");
    exit(9);
  }
}

// Called when an app exits
static void handle_app_finished(int signum, siginfo_t *info, void *context) {
  // Block signals
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);
  sigaddset(&mask, SIGCHLD);
  if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
    fprintf(stderr, "Signal blocking error\n");
    exit(9);
  }

  int app_id = pid_to_appid(info->si_pid);
  int app_index = app_id - 1;

  write_log("Kernel got finished app %d with status %d", app_id,
            info->si_status);

  assert(apps[app_index].state == RUNNING);

  apps[app_index].state = FINISHED;

  if (all_apps_finished()) {
    // kill intersim and stop the pipe reading loop
    write_msg("All apps finished, stopping kernel");
    kernel_running = false;
    kill(intersim_pid, SIGTERM);
  }

  // Unblock signals
  if (sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0) {
    fprintf(stderr, "Signal blocking error\n");
    exit(9);
  }
}

// Called on Ctrl+C.
// Terminate children, cleanup and exit
static void handle_sigint(int signum) {
  write_msg("Kernel stopping from SIGINT");

  // kill all apps
  for (int i = 0; i < APP_AMOUNT; i++) {
    kill(apps[i].app_pid, SIGTERM);
  }

  // kill intersim
  kill(intersim_pid, SIGTERM);

  // and exit from main
  kernel_running = false;
}

// Stops the current app and looks for the next available one to continue
static void schedule_next_app(void) {
  // Block signals
  // This eliminates some concurrency issues
  // sigset_t mask;
  // sigemptyset(&mask);
  // sigaddset(&mask, SIGUSR1);
  // sigaddset(&mask, SIGCHLD);
  // if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
  //   fprintf(stderr, "Signal blocking error\n");
  //   exit(9);
  // }

  // Check if we're done
  if (all_apps_finished()) {
    // kill intersim and stop the pipe reading loop
    write_msg("All apps finished, stopping kernel");
    kernel_running = false;
    kill(intersim_pid, SIGTERM);

    // // Unblock signals
    // if (sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0) {
    //   fprintf(stderr, "Signal blocking error\n");
    //   exit(9);
    // }
    return;
  }
  // stop current app if there is one, and if it isn't the last one
  int current_app_id = get_running_appid();

  if (current_app_id != -1 && get_amount_finished() < (APP_AMOUNT - 1) &&
      get_app_counter(shm, current_app_id) < APP_MAX_PC) {
    assert(apps[current_app_id - 1].state == RUNNING);
    apps[current_app_id - 1].state = PAUSED;
    kill(apps[current_app_id - 1].app_pid, SIGUSR1);
    write_msg("Kernel scheduler stopped app %d", current_app_id);
  } else {
    write_log("Kernel scheduler found no app to stop");
  }

  // continue next available app
  for (int i = 0; i < APP_AMOUNT; i++) {
    assert(schedule_next_app_index >= 0 &&
           schedule_next_app_index < APP_AMOUNT);

    // Don't continue the app we just stopped
    if (schedule_next_app_index == (current_app_id - 1))
      continue;

    if (apps[schedule_next_app_index].state == PAUSED) {
      // found app ready to be continued
      kill(apps[schedule_next_app_index].app_pid, SIGCONT);
      apps[schedule_next_app_index].state = RUNNING;

      write_msg("Kernel scheduler continued app %d",
                apps[schedule_next_app_index].app_id);

      schedule_next_app_index = (schedule_next_app_index + 1) % APP_AMOUNT;
      // // Unblock signals
      // if (sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0) {
      //   fprintf(stderr, "Signal blocking error\n");
      //   exit(9);
      // }
      return;
    }

    schedule_next_app_index = (schedule_next_app_index + 1) % APP_AMOUNT;

    // // Unblock signals
    // if (sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0) {
    //   fprintf(stderr, "Signal blocking error\n");
    //   exit(9);
    // }
  }

  // If we get here, it means there was no app to continue
  write_log("Kernel scheduler found no paused apps to continue");
}

int main(void) {
  srand(time(NULL));
  write_log("\n\n----------------------------------------------------");
  write_log("Kernel booting");
  // Validate some configs
  assert(APP_AMOUNT >= 3 && APP_AMOUNT <= 5);
  assert(APP_MAX_PC > 0);
  assert(APP_SLEEP_TIME_MS > 0);
  assert(APP_SYSCALL_PROB >= 0 && APP_SYSCALL_PROB <= 100);

  // Register signal handlers
  struct sigaction sa_syscall;
  // This flag lets us receive the child pid
  sa_syscall.sa_flags = SA_SIGINFO;
  sa_syscall.sa_sigaction = handle_app_syscall;
  if (sigaction(SIGUSR1, &sa_syscall, NULL) == -1) {
    fprintf(stderr, "Signal error\n");
    exit(4);
  }
  struct sigaction sa_finished;
  // Don't call SIGCHLD for SIGSTOPs, only when it actually terminates
  sa_finished.sa_flags = SA_SIGINFO | SA_NOCLDSTOP;
  sa_finished.sa_sigaction = handle_app_finished;
  if (sigaction(SIGCHLD, &sa_finished, NULL) == -1) {
    fprintf(stderr, "Signal error\n");
    exit(4);
  }
  if (signal(SIGINT, handle_sigint) == SIG_ERR) {
    fprintf(stderr, "Signal error\n");
    exit(4);
  }

  // Allocate shared memory to communicate with apps
  int shm_id = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | S_IRWXU);
  if (shm_id < 0) {
    fprintf(stderr, "Shm alloc error\n");
    exit(3);
  }

  shm = (int *)shmat(shm_id, NULL, 0);
  memset(shm, 0, SHM_SIZE);

  // Allocate device waiting queues
  D1_app_queue = create_queue();
  D2_app_queue = create_queue();

  // Spawn apps
  for (int i = 0; i < APP_AMOUNT; i++) {
    pid_t pid = fork();
    if (pid < 0) {
      fprintf(stderr, "Fork error\n");
      exit(2);
    } else if (pid == 0) {
      // child
      // passing shm_id and app_id as args
      char shm_id_str[10];
      char app_id_str[10];
      sprintf(shm_id_str, "%d", shm_id);
      sprintf(app_id_str, "%d", i + 1);

      execlp("./app", "app", shm_id_str, app_id_str, NULL);
    }

    apps[i].app_id = i + 1;
    apps[i].app_pid = pid;
    apps[i].D1_access_count = 0;
    apps[i].D2_access_count = 0;
    apps[i].read_count = 0;
    apps[i].write_count = 0;
    apps[i].exec_count = 0;
    apps[i].state = PAUSED;
  }

  // Create interrupts pipe
  int interpipe_fd[2];
  if (pipe(interpipe_fd) == -1) {
    fprintf(stderr, "Pipe error\n");
    exit(8);
  }

  // Spawn intersim
  intersim_pid = fork();
  if (intersim_pid < 0) {
    fprintf(stderr, "Fork error\n");
    exit(2);
  } else if (intersim_pid == 0) {
    // child
    // passing pipe fds as args
    char pipe_read_str[10];
    char pipe_write_str[10];
    sprintf(pipe_read_str, "%d", interpipe_fd[PIPE_READ]);
    sprintf(pipe_write_str, "%d", interpipe_fd[PIPE_WRITE]);

    execlp("./intersim", "intersim", pipe_read_str, pipe_write_str, NULL);
  }

  close(interpipe_fd[PIPE_WRITE]); // close write

  // Wait for all processes to boot, start intersim
  sleep(1);
  kernel_running = true;
  write_log("Kernel running");
  kill(intersim_pid, SIGCONT);

  // Start first app process manually
  apps[0].state = RUNNING;
  kill(apps[0].app_pid, SIGCONT);

  // Main loop for reading interrupt pipes
  while (kernel_running) {
    irq_t irq;
    read(interpipe_fd[PIPE_READ], &irq, sizeof(irq_t));

    if (irq == IRQ_TIME) {
      write_log("Kernel received timeslice interrupt");

      schedule_next_app();
    } else {
      assert(irq == IRQ_D1 || irq == IRQ_D2);
      write_log("Kernel received device interrupt D%d", irq);

      // Dequeue app and change its blocked state
      int app_id =
          (irq == IRQ_D1) ? dequeue(D1_app_queue) : dequeue(D2_app_queue);

      if (app_id == -1)
        continue; // queue is empty

      write_log("Kernel unblocking app %d at state %s", app_id,
                PROC_STATE_STR[apps[app_id - 1].state]);

      assert(apps[app_id - 1].state == BLOCKED);
      apps[app_id - 1].state = PAUSED;

      write_log("Kernel unblocked app %d", app_id);
    }
  }

  // Cleanup
  free_queue(D1_app_queue);
  free_queue(D2_app_queue);
  shmdt(shm);
  shmctl(shm_id, IPC_RMID, NULL);
  close(interpipe_fd[PIPE_READ]);
  write_log("Kernel finished");

  return 0;
}
