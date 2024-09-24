#include "cfg.h"
#include "types.h"
#include "util.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Called by parent on Ctrl+C or all apps finished.
// Cleanup and exit
static void handle_sigterm(int signum) {
  write_msg("Intersim stopping from SIGTERM");
  // TODO: cleanup pipes and stuff
}

int main(void) {
  srand(time(NULL));
  if (signal(SIGTERM, handle_sigterm) == SIG_ERR) {
    fprintf(stderr, "Signal error\n");
    exit(4);
  }

  // TODO: pipes

  return 0;
}
