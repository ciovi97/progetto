#include "parse_env.h"
#define FILENAME_ENV "env.conf"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#define SCALL_N(r, c, e) do { if((r = c) == NULL) { perror(e); exit(EXIT_FAILURE); } } while(0)
void carica_env(env_t *e)
{
    FILE* fp;
    SCALL_N(fp, fopen(FILENAME_ENV, "r"), "Errore apertura file di emergenze");
    char line[256];
    char* nome;
    int width, height;
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line[0] == '#' || line[0] == '\n')
            continue;

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
        } else if (strcmp(key, "width") == 0) {
            width = atoi(value);
            if (width <= 0) {
                fprintf(stderr, "Valore non valido per width: %s\n", value);
                continue;
            }
            e->width = width;
        } else if (strcmp(key, "height") == 0) {
            height = atoi(value);
            if (height <= 0) {
                fprintf(stderr, "Valore non valido per height: %s\n", value);
                continue;
            }
            e->height = height;
        } else {
            fprintf(stderr, "Chiave sconosciuta: %s\n", key);
        }
    }
 fclose(fp);
 printf("Ambiente caricato: queue=%s, width=%d, height=%d\n", 
        e->queue, e->width, e->height);
 return 0;       
}