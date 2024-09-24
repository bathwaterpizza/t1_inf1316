#include "cfg.h"
#include "types.h"
#include "util.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Called by parent on Ctrl+C or all apps finished.
// Cleanup and exit
static void handle_sigterm(int signum) {
  write_msg("Intersim stopping from SIGTERM");
  // TODO: cleanup pipes and stuff
}

int main(int argc, char **argv) {
  write_log("Intersim booting");
  srand(time(NULL));
  if (signal(SIGTERM, handle_sigterm) == SIG_ERR) {
    fprintf(stderr, "Signal error\n");
    exit(4);
  }
  // Pipe from parent
  int interpipe_fd[] = {atoi(argv[1]), atoi(argv[2])};
  close(interpipe_fd[PIPE_READ]); // close read

  // TODO: pipes

  // Start paused
  raise(SIGSTOP);

  write_log("Intersim running");

  write_log("Intersim finished");
  close(interpipe_fd[PIPE_WRITE]);
  return 0;
}
