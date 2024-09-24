#include "util.h"
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

syscall_t rand_syscall(void) {
  int r = rand();

  switch (r % 6) {
  case 0:
    return SYSCALL_D1_R;
  case 1:
    return SYSCALL_D1_W;
  case 2:
    return SYSCALL_D1_X;
  case 3:
    return SYSCALL_D2_R;
  case 4:
    return SYSCALL_D2_W;
  case 5:
    return SYSCALL_D2_X;
  default:
    fprintf(stderr, "rand_syscall error\n");
    exit(5);
  }
}

queue_t *create_queue(void) {
  queue_t *q = (queue_t *)malloc(sizeof(queue_t));
  if (q == NULL) {
    fprintf(stderr, "Malloc error\n");
    exit(6);
  }

  q->front = q->rear = NULL;

  return q;
}

void free_queue(queue_t *q) {
  node_t *current = q->front;
  node_t *next;

  while (current != NULL) {
    next = current->next;
    free(current);
    current = next;
  }

  free(q);
}

void enqueue(queue_t *q, int value) {
  node_t *temp = (node_t *)malloc(sizeof(node_t));
  if (temp == NULL) {
    fprintf(stderr, "Malloc error\n");
    exit(6);
  }
  temp->data = value;
  temp->next = NULL;

  if (q->rear == NULL) {
    q->front = q->rear = temp;
    return;
  }

  q->rear->next = temp;
  q->rear = temp;
}

int dequeue(queue_t *q) {
  if (q->front == NULL)
    return -1;

  node_t *temp = q->front;
  int value = temp->data;

  q->front = q->front->next;

  if (q->front == NULL)
    q->rear = NULL;

  free(temp);

  return value;
}
