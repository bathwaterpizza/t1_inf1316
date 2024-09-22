#include "cfg.h"
#include "types.h"
#include <assert.h>

int main(void) {
  assert(KSIM_APP_AMOUNT >= 3 && KSIM_APP_AMOUNT <= 5);

  return 0;
}
