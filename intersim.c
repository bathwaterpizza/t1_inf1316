#include "cfg.h"
#include "types.h"
#include "util.h"
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static bool intersim_running;

// Called by parent on Ctrl+C or all apps finished.
// Cleanup and exit
static void handle_sigterm(int signum) {
  write_msg("Intersim stopping from SIGTERM");

  intersim_running = false;
}

int main(int argc, char **argv) {
  write_log("Intersim booting");
  assert(argc == 3);
  srand(time(NULL));
  if (signal(SIGTERM, handle_sigterm) == SIG_ERR) {
    fprintf(stderr, "Signal error\n");
    exit(4);
  }

  // Get pipe fds from parent
  int interpipe_fd[] = {atoi(argv[1]), atoi(argv[2])};
  close(interpipe_fd[PIPE_READ]); // close read

  // Start paused
  raise(SIGSTOP);

  intersim_running = true;
  write_log("Intersim running");

  // Main loop
  while (intersim_running) {
    // TODO: write pipes
  }

  close(interpipe_fd[PIPE_WRITE]);
  write_log("Intersim finished");

  return 0;
}
