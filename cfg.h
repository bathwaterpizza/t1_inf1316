#pragma once

#define DEBUG // Show debug logging on console

#define APP_AMOUNT 5 // How many application processes should be created
#define APP_MAX_PC 5 // Program counter value at which the app process finishes
#define APP_SLEEP_TIME_MS 871 // How long should one iteration of each app take
#define APP_SYSCALL_PROB                                                       \
  15 // Percentage chance of app sending a syscall for each iteration

#define INTERSIM_SLEEP_TIME_MS 329 // How long to generate an interrupt
#define INTERSIM_D1_INT_PROB                                                   \
  20 // Percentage chance of generating a D1 interrupt for each intersim
     // iteration
#define INTERSIM_D2_INT_PROB                                                   \
  20 // Percentage chance of generating a D2 interrupt for each intersim
     // iteration

#define APP_SHM_SIZE (sizeof(int) * 2)       // Size of shm for each app process
#define SHM_SIZE (APP_SHM_SIZE * APP_AMOUNT) // Total size of shm segment

#define MAX_MSG_LEN 1024    // Max message length for logging utility
#define LOG_FILE "logs.log" // Log file name
