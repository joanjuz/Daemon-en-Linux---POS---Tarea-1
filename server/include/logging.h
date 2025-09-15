#pragma once
void log_open(const char *path);
void log_msg(const char *lvl, const char *fmt, ...);
void log_close(void);
