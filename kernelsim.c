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

// Called when kernelsim receives a syscall from one of the apps
static void handle_app_syscall(int signum, siginfo_t *info, void *context) {
  int app_id = pid_to_appid(info->si_pid);
  int app_index = app_id - 1;
  // todo
  // here we must set appinfo.syscall_handled to true,
  // and of course check if already syscall_handled when checking all apps for a
  // pending syscall

  // increment proc_info counters accordingly

  // when handling, set appinfo.state to blocked,
  // also add app to the correct device queue

  // dont forget logging
}

// Called when an app exits
static void handle_app_finished(int signum, siginfo_t *info, void *context) {
  int app_id = pid_to_appid(info->si_pid);
  int app_index = app_id - 1;

  apps[app_index].state = FINISHED;

  if (all_apps_finished()) {
    // kill intersim and stop reading the pipe
    write_msg("All apps finished, stopping kernel");
    kernel_running = false;
    kill(intersim_pid, SIGTERM);
  }
}

int main(void) {
  write_log("Kernel booting");
  // Validate some configs
  assert(APP_AMOUNT >= 3 && APP_AMOUNT <= 5);
  assert(APP_MAX_PC > 0);
  assert(APP_SLEEP_TIME_MS > 0);
  assert(APP_SYSCALL_PROB >= 0 && APP_SYSCALL_PROB <= 100);

  srand(time(NULL));

  // Register signal handlers
  struct sigaction sa_syscall;
  sa_syscall.sa_flags = SA_SIGINFO;
  sa_syscall.sa_sigaction = handle_app_syscall;
  if (sigaction(SIGUSR1, &sa_syscall, NULL) == -1) {
    fprintf(stderr, "Signal error\n");
    exit(4);
  }
  struct sigaction sa_finished;
  sa_finished.sa_flags = SA_SIGINFO;
  sa_finished.sa_sigaction = handle_app_finished;
  if (sigaction(SIGCHLD, &sa_finished, NULL) == -1) {
    fprintf(stderr, "Signal error\n");
    exit(4);
  }

  // Allocate shared memory to communicate with apps
  int shm_id = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | S_IRWXU);
  if (shm_id < 0) {
    fprintf(stderr, "Shm alloc error\n");
    exit(3);
  }

  int *shm = (int *)shmat(shm_id, NULL, 0);
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

  // Wait for all processes to boot
  sleep(1);
  kernel_running = true;
  write_log("Kernel running");

  // Main loop for reading interrupt pipes
  while (kernel_running) {
    // todo
  }

  // cleanup
  free_queue(D1_app_queue);
  free_queue(D2_app_queue);
  shmdt(shm);
  shmctl(shm_id, IPC_RMID, NULL);
  write_log("Kernel finished");

  return 0;
}
