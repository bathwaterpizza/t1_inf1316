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
  assert(get_app_syscall(shm, app_id) == SYSCALL_NONE);

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

static void send_syscall(syscall_t call) {
  // There should be no pending syscalls
  assert(get_app_syscall(shm, app_id) == SYSCALL_NONE);

  write_log("App %d started syscall: %s", app_id, SYSCALL_STR[call]);
  // todo
  // this will change the syscall status, and interrupt parent process
  // getppid() will give the parent process id
}

int main(int argc, char **argv) {
  int shm_id = atoi(argv[1]);
  app_id = atoi(argv[2]);
  counter = 0;
  write_log("App %d booting", app_id);
  assert(app_id >= 0 && app_id <= 5);

  // Register signal callbacks
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

  // Begin paused
  raise(SIGSTOP);

  // Main application loop
  while (counter < APP_MAX_PC) {
    // todo
    // probably more utils to generate probability of syscall and stuff
    // static syscall function
    sleep(1);

    counter++;
  }

  // kernelsim should use SIGCHLD to detect that the app has exited

  // cleanup
  shmdt(shm);
  write_log("App %d exiting", app_id);
  return 0;
}
