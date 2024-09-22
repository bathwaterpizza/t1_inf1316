#pragma once

// Writes to log file, and stdin if DEBUG is defined
void write_log(const char *format, ...);
// Writes to stdin and log file
void write_msg(const char *format, ...);
