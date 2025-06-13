#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "parse_rescuers.h"
#include "parse_env.h"
#include "parse_emergency_type.h"
#define FILENAME_R "rescuer.conf"
#define FILENAME_E "emergency_type.conf"
#define MAX_TIPES 10
#define MAX_EM 10
#define MAX_R 10
#include<time.h>
#ifndef PARSE_EMERGENCY_H
#define PARSE_EMERGENCY_H
#define EMERGENCY_NAME_LENGTH 64
/*typedef enum{
    WAITING , ASSIGNED , IN_PROGRESS , PAUSED , COMPLETED , CANCELED , TIMEOUT
    } emergency_status_t ;
    typedef struct {
    char emergency_name [ EMERGENCY_NAME_LENGTH ];
    int x ;
    int y ;
    time_t timestamp ;
    }emergency_request_t ;
    typedef struct {
    //emergency_type_t type ;
    emergency_status_t status ;
    int x ;
    int y ;
    time_t time ;
    int rescuer_count ;
    rescuer_digital_twin_t* rescuers_dt ;
}emergency_t ;
*/
#endif
int main() {
    rescuer_digital_twin_t *dt=malloc(sizeof(rescuer_digital_twin_t) * 200);
    rescuer_type_t *tr=malloc(sizeof(rescuer_type_t) * MAX_TIPES);
    rescuer_request_t *re=malloc(sizeof(rescuer_request_t) * MAX_R);
    emergency_type_t *te=malloc(sizeof(emergency_type_t) * MAX_EM);
    int* unità = malloc(sizeof(int) * MAX_TIPES);
    int nres=0;
    if (!dt) {
        perror("Errore nel malloc twins\n");
        return -1;
    }
    env_t env;
    env.width = 500;
    env.height = 300;
    env.queue = "emergenze654900";
    int ntot;
    printf("MAIN\n");
    if ((ntot=carica_rescuers(dt, env,tr,unità))>0) {
        printf("✅ Caricati %d soccorritori:\n\n", ntot);
    } else {
        fprintf(stderr, "Errore nel caricamento dei soccorritori.\n");
    }
    //printf("Ambulanze disponibili:%d\n",res_disponibili(dt, "Ambulanza",nres));
    printf("posizione (%d,%d) è %d\n",dt[4].x,dt[4].y,dt[4].status);
    aggiorna_posizione(&dt[4],6,6);
    modifica_stato(&dt[4],ON_SCENE);
    printf("posizione (%d,%d) è %d\n",dt[4].x,dt[4].y,dt[4].status);
    carica_em(te,re,tr,sizeof(unità));
    return 0;
}
