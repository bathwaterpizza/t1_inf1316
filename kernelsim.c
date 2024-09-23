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
#include <unistd.h>

// Whether the kernel is running and reading the interrupt controller pipe
static bool kernel_running = true;

// Called when kernelsim receives a syscall from one of the apps
static void handle_app_syscall(int signum) {
  // todo
  // here we must set appinfo.syscall_handled to true,
  // and of course check if already syscall_handled when checking all apps for a
  // pending syscall

  // when handling, set appinfo.state to blocked,
  // also add app to the correct device queue

  // dont forget logging
}

// Called when an app exits
static void handle_app_finished(int signum) {
  // todo
  // set appinfo.state to finished
  // finished apps should not be scheduled, obviously
  // if all apps are finished, set kernel_running to false?
  // it says kernelsim is an infinite process though, we'll ask

  // dont forget logging
}

int main(void) {
  write_log("Kernel booting");
  // Validate some configs
  assert(APP_AMOUNT >= 3 && APP_AMOUNT <= 5);
  assert(APP_MAX_PC > 0);
  assert(APP_SLEEP_TIME_MS > 0);

  // Register signal handlers
  if (signal(SIGUSR1, handle_app_syscall) == SIG_ERR) {
    fprintf(stderr, "Signal error\n");
    exit(4);
  }
  if (signal(SIGCHLD, handle_app_finished) == SIG_ERR) {
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

  // Spawn apps
  proc_info_t apps[APP_AMOUNT];

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
    apps[i].syscall_handled = false;
  }

  // Wait for all processes to boot
  sleep(1);
  write_log("Kernel running");

  // when receiving D1 or D2 interrupt and popping the process waiting on a
  // syscall from queue, make sure to set appinfo.syscall_handled to false

  // Main loop for reading interrupt pipes
  while (kernel_running) {
    // todo
  }

  // cleanup
  shmdt(shm);
  shmctl(shm_id, IPC_RMID, NULL);
  write_log("Kernel finished");
  return 0;
}
