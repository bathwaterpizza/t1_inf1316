#include "cfg.h"
#include "types.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void write_log(const char *format, ...) {
  FILE *file = fopen(LOG_FILE, "a");
  if (file == NULL) {
    fprintf(stderr, "Log file error\n");
    exit(1);
  }

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  char datetime_str[20];
  strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M:%S", t);

  char time_str[10];
  strftime(time_str, sizeof(time_str), "%H:%M:%S", t);

  fprintf(file, "[%s] ", datetime_str);
#ifdef DEBUG
  printf("[%s] ", time_str);
#endif

  va_list args;
  va_start(args, format);

  char message[MAX_MSG_LEN];
  vsnprintf(message, sizeof(message), format, args);

  fprintf(file, "%s\n", message);
#ifdef DEBUG
  printf("%s\n", message);
#endif

  va_end(args);
  fclose(file);
}

void write_msg(const char *format, ...) {
  FILE *file = fopen(LOG_FILE, "a");
  if (file == NULL) {
    fprintf(stderr, "Log file error\n");
    exit(1);
  }

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  char datetime_str[20];
  strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M:%S", t);

  fprintf(file, "[%s] ", datetime_str);

  va_list args;
  va_start(args, format);

  char message[MAX_MSG_LEN];
  vsnprintf(message, sizeof(message), format, args);

  fprintf(file, "%s\n", message);
  printf("%s\n", message);

  va_end(args);
  fclose(file);
}

int get_app_counter(int *shm, int app_id) {
  assert(shm != NULL);
  return *(shm + ((app_id - 1) * 2));
}

syscall_t get_app_syscall(int *shm, int app_id) {
  assert(shm != NULL);
  return *(shm + 1 + ((app_id - 1) * 2));
}

void set_app_counter(int *shm, int app_id, int value) {
  assert(shm != NULL);
  *(shm + ((app_id - 1) * 2)) = value;
}

void set_app_syscall(int *shm, int app_id, syscall_t call) {
  assert(shm != NULL);
  *(shm + 1 + ((app_id - 1) * 2)) = (int)call;
}
