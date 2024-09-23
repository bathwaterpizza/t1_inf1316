#pragma once

#include <sys/types.h>

// Interruptions generated by the InterController Sim
typedef enum {
  IRQ_TIME, // Timeslice finished
  IRQ_D1,   // Device D1 interruption
  IRQ_D2    // Device D2 interruption
} irq_t;

// System calls requested by application process
typedef enum {
  SYSCALL_NONE, // No syscall requested
  SYSCALL_D1_R, // Read from device D1
  SYSCALL_D1_W, // Write to device D1
  SYSCALL_D2_R, // Read from device D2
  SYSCALL_D2_W  // Write to device D2
} syscall_t;

// String version of the syscalls
extern const char *SYSCALL_STR[];

// Application process states
typedef enum {
  RUNNING, // Process is active
  BLOCKED, // Process is waiting for device interrupt
  PAUSED,  // Process is waiting for a SIGCONT
  FINISHED // Process has finished executing (PC >= APP_MAX_PC)
} proc_state_t;

// Contains information about each application process
typedef struct {
  int app_id; // Internal app ID, starting from 1
  pid_t app_pid;
  int D1_access_count; // Amount of syscalls to D1
  int D2_access_count; // Amount of syscalls to D2
  int read_count;      // Amount of R syscalls
  int write_count;     // Amount of W syscalls
  proc_state_t
      state; // Current state of the process, according to the kernelsim
} proc_info_t;
