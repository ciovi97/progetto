#include <stdio.h>
#include <time.h>
#include "log.h"

void scrivi_log(time_t now,mtx_t *log_mutex, const char *id, const char *evento, const char *msg) {
    FILE *fp;

    mtx_lock(log_mutex);
    fp = fopen("log.txt", "a");
    if (fp) {
        fprintf(fp, "[%ld] [%s] [%s] %s\n", now, id, evento, msg);
        fclose(fp);
    }
    mtx_unlock(log_mutex);
}