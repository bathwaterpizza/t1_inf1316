#pragma once

#define DEBUG // Show debug logging on console

#define APP_AMOUNT 3  // How many application processes should be created
#define APP_MAX_PC 10 // Program counter value at which the app process finishes
#define APP_SLEEP_TIME_MS 1000 // How long should one iteration of each app take

#define APP_SHM_SIZE (sizeof(int) * 2)       // Size of shm for each app process
#define SHM_SIZE (APP_SHM_SIZE * APP_AMOUNT) // Total size of shm segment

#define MAX_MSG_LEN 1024    // Max message length for logging utility
#define LOG_FILE "logs.log" // Log file name
