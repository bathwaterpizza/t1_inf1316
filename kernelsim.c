#include "cfg.h"
#include "types.h"
#include "util.h"
#include <assert.h>

int main(void) {
  assert(APP_AMOUNT >= 3 && APP_AMOUNT <= 5);

  return 0;
}
