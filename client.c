#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <threads.h>

 #include "parse_env.h"
 #include "parse_emergency_type.h"
 #include "emergency.h"
 #include "log.h"

 #define MAX_LINE 256
 #define MAX_EM 10


 // Funzione per inviare una emergenza alla message queue
 void invia_emergenza(mqd_t coda, emergency_request_t *req) {
     if (mq_send(coda, (char*)req, sizeof(*req), 0) == -1) {
         perror("mq_send");
     }
 }


int main(int argc, char *argv[]) {
   printf("[INFO] Avvio client...\n");
   env_t env_struct;
   env_t* env = &env_struct;
   mtx_t mutex_log;
    if (mtx_init(&mutex_log, mtx_plain) != thrd_success) {
        fprintf(stderr, "[ERRORE] Inizializzazione mutex_log fallita\n");
        return EXIT_FAILURE;
    }

     // Caricamento configurazione da file
    carica_env(env, &mutex_log);
     // Impostazioni coda POSIX
     struct mq_attr attr = {
         .mq_flags = 0,
         .mq_maxmsg = 10,
         .mq_msgsize = sizeof(emergency_request_t),
         .mq_curmsgs = 0
     };

     // Apertura coda di messaggi per invio (scrittura)
     mqd_t coda = mq_open(env->queue, O_WRONLY | O_CREAT, 0644, &attr);
     if (coda == (mqd_t)-1) {
         perror("mq_open");
         exit(EXIT_FAILURE);
     }

     // --- Modalita' SINGOLA EMERGENZA ---
     if (argc == 5) {
         const char *nome = argv[1];
         int x = atoi(argv[2]);
         int y = atoi(argv[3]);
         int delay = atoi(argv[4]);

         emergency_request_t req;
         int id = (int)time(NULL);  // id temporaneo basato sul timestamp
         strncpy(req.emergency_name, nome, sizeof(req.emergency_name) - 1);
         req.emergency_name[sizeof(req.emergency_name) - 1] = '\0';
         req.x = x;
         req.y = y;
         req.timestamp = time(NULL) + delay;

         char idbuf[32];
         snprintf(idbuf, sizeof(idbuf), "%d", id);

         // Validazione dei parametri prima di inviare
         if (x < 0 || y < 0 || x >= env->height || y >= env->width || delay < 0) {
             char msg[128];
             snprintf(msg, sizeof(msg), "Emergenza non valida (%s %d %d %d)", nome, x, y, delay);
             scrivi_log(time(NULL), &mutex_log, idbuf, "CLIENT", msg);
         } else {
             sleep(delay); // attesa simulata
             invia_emergenza(coda, &req);
         }

     // --- Modalita' FILE ---
     } else if (argc == 3 && strcmp(argv[1], "-f") == 0) {
         FILE *fp = fopen(argv[2], "r");
         if (!fp) {
             perror("fopen");
             exit(EXIT_FAILURE);
         }
         char line[MAX_LINE];

         // Lettura linea per linea del file
         while (fgets(line, sizeof(line), fp)) {
             char nome[64];
             int x, y, delay;

             // Parsing della riga
             if (sscanf(line, "%s %d %d %d", nome, &x, &y, &delay) != 4) {
                scrivi_log(time(NULL), &mutex_log, "[N/A]", "CLIENT", "Formato riga non valido");
                 continue;
             }

             emergency_request_t req;
             int id = (int)time(NULL); // id generato
             strncpy(req.emergency_name, nome, sizeof(req.emergency_name) - 1);
             req.emergency_name[sizeof(req.emergency_name) - 1] = '\0';
             req.x = x;
             req.y = y;
             req.timestamp = time(NULL) + delay;

             char idbuf[32];
             snprintf(idbuf, sizeof(idbuf), "%d", id);

             // Validazione
             if (x < 0 || y < 0 || x >= env->height || y >= env->width || delay < 0) {
                 char msg[128];
                 snprintf(msg, sizeof(msg), "Emergenza non valida da file (%s %d %d %d)", nome, x, y, delay);
                scrivi_log(time(NULL), &mutex_log, idbuf, "CLIENT", msg);
             } else {
                 sleep(delay);
                 invia_emergenza(coda, &req);
             }
         }
         fclose(fp);

     // --- Caso errore nei parametri ---
     } else {
         fprintf(stderr, "Uso: ./client <nome_emergenza> <x> <y> <ritardo>\n   ./client -f <file>\n");
         exit(EXIT_FAILURE);
     }

     mq_close(coda); // chiusura coda
    mtx_destroy(&mutex_log);
     return 0;
 }
