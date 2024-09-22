#include "cfg.h"
#include "types.h"
#include "util.h"
#include <assert.h>

int main(void) {
  assert(APP_AMOUNT >= 3 && APP_AMOUNT <= 5);

  write_log("testing log only, int is %d", 10);
  write_msg("testing log and msg, int is %d", 15);

  return 0;
}
