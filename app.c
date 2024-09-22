#include "cfg.h"
#include "types.h"
#include "util.h"
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <unistd.h>

static int *shm;
static int app_id;
static int counter;

// Called when app receives SIGUSR1 from kernelsim
// Saves state in shm and raises SIGSTOP
static void handle_kernel_stop(int signum) {
  write_msg("App %d stopped", app_id);
}

// Called when app receives SIGCONT from kernelsim
// Restores state from shm
static void handle_kernel_cont(int signum) {
  write_msg("App %d resumed", app_id);
  // restore program counter
}

int main(int argc, char **argv) {
  int shm_id = atoi(argv[1]);
  app_id = atoi(argv[2]);
  counter = 0;
  write_log("App %d booting", app_id);
  assert(app_id >= 0 && app_id <= 5);

  // Register signal callback
  if (signal(SIGUSR1, handle_kernel_stop) == SIG_ERR) {
    fprintf(stderr, "Signal error\n");
    exit(4);
  }
  if (signal(SIGCONT, handle_kernel_cont) == SIG_ERR) {
    fprintf(stderr, "Signal error\n");
    exit(4);
  }

  // Attach to shm
  shm = (int *)shmat(shm_id, NULL, 0);

  // Start paused
  kill(getpid(), SIGSTOP);

  // Main application loop
  while (counter < APP_MAX_PC) {
    // todo
  }

  // cleanup
  shmdt(shm);
  write_log("App %d exiting", app_id);
  return 0;
}
