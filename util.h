#pragma once

#include "types.h"

// Writes to log file, and stdin if DEBUG is defined,
// Inserts newline ending
void write_log(const char *format, ...);

// Writes to stdin and log file,
// Inserts newline ending
void write_msg(const char *format, ...);

// Get program counter value from shm for the given app_id
int get_app_counter(int *shm, int app_id);

// Get syscall request status from shm for the given app_id
syscall_t get_app_syscall(int *shm, int app_id);

// Set program counter value in shm for the given app_id
void set_app_counter(int *shm, int app_id, int value);

// Set syscall request status in shm for the given app_id
void set_app_syscall(int *shm, int app_id, syscall_t call);

// Generate a random syscall from options (D1/D2, R/W/X)
syscall_t rand_syscall(void);
