#include "cfg.h"
#include "types.h"
#include "util.h"
#include <assert.h>
#include <stdlib.h>
#include <sys/shm.h>

int main(int argc, char **argv) {
  int shm_id = atoi(argv[1]);
  int app_id = atoi(argv[2]);
  int counter = 0;
  write_log("App %d booting", app_id);
  assert(app_id >= 0 && app_id <= 5);

  // Attach to shm
  int *shm = (int *)shmat(shm_id, NULL, 0);

  // cleanup
  shmdt(shm);
  write_log("App %d exiting", app_id);
  return 0;
}
