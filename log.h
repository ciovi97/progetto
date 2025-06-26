#ifndef LOG_H
#define LOG_H
#include <threads.h>
#include <time.h>
void scrivi_log(time_t timestamp,mtx_t *log_mutex, const char *id, const char *evento, const char *msg);
#endif