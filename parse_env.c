#include "parse_env.h"
#define FILENAME_ENV "env.conf"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#define SCALL_N(r, c, e) do { if((r = c) == NULL) { perror(e); exit(EXIT_FAILURE); } } while(0)
int carica_env(env_t *e,mtx_t *mutex)
{
    FILE* fp;
    SCALL_N(fp, fopen(FILENAME_ENV, "r"), "Errore apertura file di emergenze\n");
    char line[256];
    char msg[300];
    char* nome;
    int par=0;
    int width, height;
    while (fgets(line, sizeof(line), fp) != NULL&&par<3) {
        if (line[0] == '#' || line[0] == '\n')
            continue;
        snprintf(msg, sizeof(msg), "Riga letta: %s", line);
        scrivi_log(time(NULL),mutex, "env.conf", "PARSING", msg);    
        char *key = strtok(line, "=\n");
        char *value = strtok(NULL, "=\n");
        if (!key || !value)
            continue;
        if (strcmp(key, "queue") == 0) {    
            e->queue = malloc(strlen(value) + 1);
            if (!e->queue) {
                perror("Errore allocazione memoria per queue");
                exit(EXIT_FAILURE);
            }
            strcpy(e->queue, value);
            par++;
        } else if (strcmp(key, "width") == 0) {
            width = atoi(value);
            if (width <= 0) {
                snprintf(msg, sizeof(msg), "Valore %s sconosciuta non valido per width", value);
                scrivi_log(time(NULL),mutex, "env.conf", "PARSING", msg);                
                continue;
            }
            e->width = width;
            par++;
        } else if (strcmp(key, "height") == 0) {
            height = atoi(value);
            if (height <= 0) {
                snprintf(msg, sizeof(msg), "Valore %s sconosciuta non valido per height", value);
                scrivi_log(time(NULL),mutex, "N/A", "PARSING", msg);
                continue;
            }
            e->height = height;
            par++;
        } else {
            snprintf(msg, sizeof(msg), "Chiave %s sconosciuta", key);
            scrivi_log(time(NULL),mutex, "env.conf", "PARSING", msg);
            continue;        
        }
    }
 fclose(fp);
    if(!(e->queue) ||!(e->width)||!(e->height)) {
        snprintf(msg, sizeof(msg), "File parse_env.conf non ben scritto");
        scrivi_log(time(NULL),mutex, "env.conf", "PARSING", msg);
        exit(EXIT_FAILURE);
    }
    snprintf(msg, sizeof(msg), "PARSING AMBIENTE COMPLETATO. Configurazione caricata: queue=%s, width=%d, height=%d", e->queue, e->width, e->height);
    scrivi_log(time(NULL),mutex, "env.conf", "PARSING", msg);
 return 0;       
}