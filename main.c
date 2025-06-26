#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <threads.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <signal.h>

#include "parse_rescuers.h"
#include "parse_env.h"
#include "parse_emergency_type.h"
#include "emergency.h"

#define FILENAME_R "rescuer.conf"
#define FILENAME_E "emergency_type.conf"
#define MAX_TIPES 10 //numero massimo di tipi di soccorritori
#define MAX_EM  50 //numero massimo di emergenze in esecuzione
#define MAX_RES 70  //numero massimo di soccorritori
#define TIME_AGING 60 // secondi per promuovere da priorità 0 a 1
#define LOTTO_TIME 5 // secondi per accumulare richieste nella pre_coda

//mutex,array e numero delle emergenze in esecuzione
emergency_t emergenze_attive[MAX_EM];
mtx_t mutex_emergenze;
int num_emergenze_attive = 0;
mtx_t mutex_em;
mtx_t mutex_emergenze_singole[MAX_EM]; // mutex per ogni emergenza in esecuzione
//array e numero max delle emergenze caricate prima di mandarle in esecuzione
#define MAX_QUEUE 100
emergency_t *preexec_queue[MAX_QUEUE];
int num_in_queue = 0;

//mutex array soccorritori
mtx_t mutex_res[MAX_RES];
volatile sig_atomic_t interrompi = 0;
void gestisci_ctrl_c(int sig) {
    (void)sig;
    interrompi = 1;
    const char msg[] = "\n[INFO] CTRL+C intercettato, avvio della terminazione...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

typedef struct {
    env_t *env;
    mtx_t *mutex_log;
    rescuer_digital_twin_t *dt;
    int nres;
    emergency_type_t *tipi;
    int max_em;
} dispatcher_args_t;
emergency_t *estrai_prima_emergenza(emergency_t **preexec_queue, int *num_in_queue) {
    if (*num_in_queue == 0) {
        return NULL; // Coda vuota, niente da estrarre
    }
    // La prima emergenza è in posizione 0 (coda ordinata)
    emergency_t *em = preexec_queue[0];

    // Sposta tutti gli altri elementi indietro di una posizione
    for (int i = 1; i < *num_in_queue; i++) {
        preexec_queue[i-1] = preexec_queue[i];
    }
    (*num_in_queue)--; // Decrementa il numero di emergenze in coda

    return em; // Restituisce la prima emergenza estratta
}
emergency_type_t* definiscitipo(char* nome,emergency_type_t *tipi,int ntipi) {
    for(int i=0;i<ntipi;i++) {
        if(strcmp(nome,tipi[i].emergency_desc)==0) {
            return &tipi[i];
        }
    }
    static const emergency_type_t no = {0};
    return &no;
}

void aging_e_sort_preexec_queue(emergency_t **preexec_queue, int num_in_queue) {
    time_t now = time(NULL);
    int changed = 0;

    // 1. Aging: promuovi priorità 0 se aspettano più di SOGLIA_AGING secondi
    for (int i = 0; i < num_in_queue; i++) {
        if (preexec_queue[i]->type.priority == 0) {
            double attesa = difftime(now, preexec_queue[i]->time);
            if (attesa >= TIME_AGING) {
                preexec_queue[i]->type.priority = 1; // promozione
                changed = 1;
            }
        }
    }

    // 2. Sorting: solo se la coda ha almeno 2 elementi
    if (num_in_queue > 1) {
        for (int i = 0; i < num_in_queue - 1; i++) {
            for (int j = 0; j < num_in_queue - 1 - i; j++) {
                emergency_t *a = preexec_queue[j];
                emergency_t *b = preexec_queue[j+1];
                if (
                    a->type.priority < b->type.priority ||
                    (a->type.priority == b->type.priority && a->time > b->time)
                ) {
                    // Scambia per avere priorità più alte davanti, a parità prima il più vecchio
                    preexec_queue[j] = b;
                    preexec_queue[j+1] = a;
                }
            }
        }
    }
}
int posizione_in_uso(int x, int y, mtx_t *mutex_emergenze) {
    int in_uso = 0;
    // Acquisizione del lock per leggere la lista in sicurezza
    mtx_lock(&mutex_emergenze);
    for (int i = 0; i < num_emergenze_attive; i++) {
        if (emergenze_attive[i].x == x && emergenze_attive[i].y == y &&
            emergenze_attive[i].status != COMPLETED && emergenze_attive[i].status != TIMEOUT && emergenze_attive[i].status != CANCELED) {
            in_uso = 1;
            break;
        }
    }
    mtx_unlock(&mutex_emergenze);
    return in_uso;
}

int dispatcher_thread(void *arg) {
    int i=0;
    dispatcher_args_t *dargs = arg;
    struct mq_attr attr = {
        .mq_flags   = O_NONBLOCK,
        .mq_maxmsg  = 10,
        .mq_msgsize = sizeof(emergency_request_t),
        .mq_curmsgs = 0
    };
    mqd_t coda = mq_open(dargs->env->queue, O_CREAT | O_RDONLY, 0644, &attr);
    if (coda == (mqd_t)-1) {
        perror("mq_open dispatcher");
        return -1;
    }

    /* breve sleep per non busy-waitare al 100% la CPU */
    struct timespec sleep_ts = { .tv_sec = 0, .tv_nsec = 100 * 1000 * 1000 }; // 100 ms

    char msg[256];
    emergency_request_t richiesta;

    while (!interrompi) {

        /*  
         * 1) Drena tutti i messaggi presenti (senza timeout)
         *    In un unico ciclo, legge fino a che mq_receive restituisce >= 0
         */
        while (mq_receive(coda, (char *)&richiesta, attr.mq_msgsize, NULL) >= 0) {
            /* log della ricezione */
            snprintf(msg, sizeof(msg),
                     "Nuova richiesta ricevuta: %s [%d,%d] @%ld",
                     richiesta.emergency_name,
                     richiesta.x, richiesta.y,
                     richiesta.timestamp);
            scrivi_log(time(NULL), dargs->mutex_log,
                       "N/A", "MESSAGE_QUEUE", msg);

            /* verifica tipo emergenza */
            emergency_type_t *type_ptr =
                definiscitipo(richiesta.emergency_name,
                              dargs->tipi, dargs->max_em);
            if (!type_ptr->emergency_desc) {
                snprintf(msg, sizeof(msg),
                         "Tipo emergenza '%s' non riconosciuto, scartata",
                         richiesta.emergency_name);
                scrivi_log(time(NULL), dargs->mutex_log,
                           "N/A", "MESSAGE_QUEUE", msg);
                continue;
            }

            /* alloca e inizializza nuova emergency_t */
            emergency_t *nuova_em = malloc(sizeof(*nuova_em));
            if (!nuova_em) { perror("malloc"); continue; }
            nuova_em->type          = *type_ptr;
            nuova_em->x             = richiesta.x;
            nuova_em->y             = richiesta.y;
            nuova_em->time          = richiesta.timestamp;
            nuova_em->status        = WAITING;
            nuova_em->rescuer_count = 0;
            nuova_em->rescuers_dt   = NULL;

            /* inserimento in preexec_queue */
            if (num_in_queue < MAX_QUEUE) {
                preexec_queue[num_in_queue++] = nuova_em;
                snprintf(msg, sizeof(msg),
                         "Inserita in pre-coda: %s [%d,%d]",
                         richiesta.emergency_name,
                         richiesta.x, richiesta.y);
                scrivi_log(time(NULL), dargs->mutex_log,
                           "N/A", "DISPATCHER", msg);
            } else {
                snprintf(msg, sizeof(msg),
                         "Pre-coda piena, scartata: %s [%d,%d]",
                         richiesta.emergency_name,
                         richiesta.x, richiesta.y);
                scrivi_log(time(NULL), dargs->mutex_log,
                           "N/A", "DISPATCHER", msg);
                free(nuova_em);
            }
        }

        /* 2) Se ho drenato almeno una richiesta, applico aging + sort */
        if(num_in_queue>1)
        aging_e_sort_preexec_queue(preexec_queue, num_in_queue);
        /* 3) Se la pre-coda non è vuota, estraggo ed eseguo la prima */
        if (num_in_queue > 0) {
            emergency_t *em_pronta =
                estrai_prima_emergenza(preexec_queue, &num_in_queue);

            /* controllo collisione di posizione con emergenze attive */
            if (!posizione_in_uso(em_pronta->x, em_pronta->y, &mutex_emergenze)) {
                /* aggiungo a emergenze_attive protetto da mutex */
                mtx_lock(&mutex_emergenze);
                emergenze_attive[num_emergenze_attive++] = *em_pronta;
                mtx_unlock(&mutex_emergenze);

                /* preparo args e lancio thread di gestione */
                gestore_args_t *gargs = malloc(sizeof(*gargs));
                if (!gargs) { perror("malloc"); free(em_pronta); continue; }
                gargs->em               = em_pronta;
                gargs->dt               = dargs->dt;
                gargs->nres             = dargs->nres;
                gargs->mutex_log        = dargs->mutex_log;
                gargs->emergenze_attive = emergenze_attive;
                gargs->num_emergenze_attive = &num_emergenze_attive;
                gargs->interrompi            = &interrompi;
                gargs->mutex_emergenze       = &mutex_emergenze;
                gargs->mutex_rescuers        = &mutex_res;
                gargs->aging_time            = TIME_AGING;
                gargs->mutex_em = &mutex_emergenze_singole[i];
                i++;
                thrd_t t;
                if (thrd_create(&t, gestisci_emergenza, gargs) != thrd_success) {
                    free(gargs);
                } else {
                    thrd_detach(t);
                }
            } else {
                /* posizione già occupata: scarto */
                snprintf(msg, sizeof(msg),
                         "Posizione [%d,%d] in uso, scarto %s",
                         em_pronta->x, em_pronta->y,
                         em_pronta->type.emergency_desc);
                scrivi_log(time(NULL), dargs->mutex_log,
                           "N/A", "DISPATCHER", msg);
                free(em_pronta);
            }
            /* vado subito al prossimo giro senza sleep per processare
             * eventuali altre emergenze già pronte */
            continue;
        }

        /* 4) Se non ci sono emergenze da avviare, attendo un breve intervallo */
        thrd_sleep(&sleep_ts, NULL);
    }
    for(int i=0; i < num_in_queue; i++) {
        free(preexec_queue[i]->rescuers_dt);
    }
    mq_close(coda);
    return 0;
}

int main() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gestisci_ctrl_c;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    rescuer_digital_twin_t *dt = malloc(sizeof(rescuer_digital_twin_t) * MAX_RES);
    if( !dt) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    rescuer_type_t *tr = malloc(sizeof(rescuer_type_t) * MAX_TIPES);
    if(!tr)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    rescuer_request_t *re = malloc(sizeof(rescuer_request_t) * MAX_RES);
    if(!re) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    emergency_type_t *te = malloc(sizeof(emergency_type_t) * MAX_EM);
    if(!te) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    int* unita = malloc(sizeof(int) * MAX_TIPES);
    if (!unita) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    env_t *env = malloc(sizeof(env_t));

    if (!env) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    

    mtx_t mutex_log;
    thrd_t dispatcher;

    if (mtx_init(&mutex_log, mtx_plain) != thrd_success) {
        fprintf(stderr, "[ERRORE] Inizializzazione mutex_log fallita\n");
        return EXIT_FAILURE;
    }

    carica_env(env, &mutex_log);
    int ntot = carica_rescuers(dt, env, tr, unita, &mutex_log);
    for (int i = 0; i < ntot; i++) {
    if (mtx_init(&mutex_res[i], mtx_plain) != thrd_success) {
        fprintf(stderr, "Errore init mutex_res[%d]\n", i);
        exit(EXIT_FAILURE);
    }
    }
    carica_em(te, re, tr, MAX_TIPES, &mutex_log);
    mtx_init(&mutex_emergenze, mtx_plain);
    for(int i=0; i < MAX_EM; i++) {
        if (mtx_init(&mutex_emergenze_singole[i], mtx_plain) != thrd_success) {
            fprintf(stderr, "Errore init mutex_emergenze_singole[%d]\n", i);
            exit(EXIT_FAILURE);
        }
    }
    dispatcher_args_t *dargs = malloc(sizeof(dispatcher_args_t));
    if(!dargs) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    dargs->env = env;
    dargs->mutex_log =&mutex_log;
    dargs->dt = dt;
    dargs->nres = ntot;
    dargs->tipi = te;
    dargs->max_em = MAX_EM;
    if (thrd_create(&dispatcher, dispatcher_thread, dargs) != thrd_success) {
        fprintf(stderr, "Errore creazione thread dispatcher\n");
        return EXIT_FAILURE;
    }

    thrd_join(dispatcher, NULL);
    mq_unlink(env->queue);

    free(dt);
    free(tr);
    free(re);
    free(te);
    free(unita);
    free(env);
    free(dargs);
    printf("[INFO] Chiusura del programma completata.\n");
    mtx_destroy(&mutex_log);
    mtx_destroy(&mutex_emergenze);
    mtx_destroy(&mutex_em);
    for (int i = 0; i < ntot; i++) {
        mtx_destroy(&mutex_res[i]);
    }
    if (interrompi) {
        printf("[INFO] Interruzione forzata del dispatcher.\n");
    } else {
        printf("[INFO] Dispatcher terminato correttamente.\n");
    }
    return 0;
}
