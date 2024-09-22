#include "cfg.h"
#include "types.h"
#include "util.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
  write_log("Kernel booting");
  // Validate some configs
  assert(APP_AMOUNT >= 3 && APP_AMOUNT <= 5);
  assert(APP_MAX_PC > 0);

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

      // exec app program, shm_id and app_id as argument
    }

    apps[i].app_id = i + 1;
    apps[i].app_pid = pid;
    apps[i].state = PAUSED;
  }

  // cleanup
  shmdt(shm);
  shmctl(shm_id, IPC_RMID, NULL);
  write_log("Kernel exiting");
  return 0;
}
