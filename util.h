#pragma once

// Writes to log file, and stdin if DEBUG is defined,
// Inserts newline ending
void write_log(const char *format, ...);

// Writes to stdin and log file,
// Inserts newline ending
void write_msg(const char *format, ...);
