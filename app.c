#include "cfg.h"
#include "types.h"
#include "util.h"
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

static int *shm;
static int app_id;
static int counter = 0;

// Called when app receives SIGUSR1 from kernelsim
// Saves state in shm and raises SIGSTOP
static void handle_kernel_stop(int signum) {
  // assert(get_app_syscall(shm, app_id) == SYSCALL_NONE);

  write_msg("App %d stopped at counter %d", app_id, counter);

  // Save program counter state to shm
  set_app_counter(shm, app_id, counter);

  // Wait for continue from kernelsim
  raise(SIGSTOP);
}

// Called when app receives SIGCONT from kernelsim
// Restores state from shm
static void handle_kernel_cont(int signum) {
  write_msg("App %d resumed at counter %d", app_id, counter);

  // Restore program counter state from shm
  counter = get_app_counter(shm, app_id);

  // Restore syscall state from shm
  if (get_app_syscall(shm, app_id) != SYSCALL_NONE) {
    // announce syscall completed and change status to none
    write_log("App %d completed syscall: %s", app_id,
              SYSCALL_STR[get_app_syscall(shm, app_id)]);
    set_app_syscall(shm, app_id, SYSCALL_NONE);
  }
}

// Called by parent on Ctrl+C.
// Cleanup and exit
static void handle_sigterm(int signum) {
  write_log("App %d stopping from SIGTERM", app_id);
  shmdt(shm);
  exit(0);
}

// Sends a syscall request to kernelsim
static void send_syscall(syscall_t call) {
  // There should be no pending syscalls
  assert(get_app_syscall(shm, app_id) == SYSCALL_NONE);

  write_log("App %d started syscall: %s", app_id, SYSCALL_STR[call]);

  // Set desired syscall and send request to kernelsim
  set_app_syscall(shm, app_id, call);
  kill(getppid(), SIGUSR1);
  pause();
}

int main(int argc, char **argv) {
  int shm_id = atoi(argv[1]);
  app_id = atoi(argv[2]);
  write_log("App %d booting", app_id);
  assert(app_id >= 0 && app_id <= 5);
  assert(argc == 3);
  srand(time(NULL)); // reset seed

  // Register signal callbacks
  if (signal(SIGUSR1, handle_kernel_stop) == SIG_ERR) {
    fprintf(stderr, "Signal error\n");
    exit(4);
  }
  if (signal(SIGCONT, handle_kernel_cont) == SIG_ERR) {
    fprintf(stderr, "Signal error\n");
    exit(4);
  }
  if (signal(SIGTERM, handle_sigterm) == SIG_ERR) {
    fprintf(stderr, "Signal error\n");
    exit(4);
  }

  // Attach to kernelsim shm
  shm = (int *)shmat(shm_id, NULL, 0);

  // Begin paused
  raise(SIGSTOP);

  write_log("App %d running", app_id);

  // Main application loop
  while (counter < APP_MAX_PC) {
    usleep((APP_SLEEP_TIME_MS / 2) * 1000);

    if (rand() % 100 < APP_SYSCALL_PROB) {
      send_syscall(rand_syscall());
    }

    counter++;
    write_log("App %d counter is now %d", app_id, counter);

    usleep((APP_SLEEP_TIME_MS / 2) * 1000);
  }

  // update context before exiting
  set_app_counter(shm, app_id, counter);

  // cleanup
  shmdt(shm);
  write_log("App %d finished", app_id);
  return 0;
}
