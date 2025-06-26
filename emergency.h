#ifndef PARSE_EMERGENCY_H
#define PARSE_EMERGENCY_H
#include "parse_emergency_type.h"
#include "parse_rescuers.h"
#include <time.h>
#include <signal.h>
#define EMERGENCY_NAME_LENGTH 64
typedef enum{
    WAITING , ASSIGNED , IN_PROGRESS , PAUSED , COMPLETED , CANCELED , TIMEOUT
    } emergency_status_t ;
    typedef struct {
    int id ;    
    char emergency_name [ EMERGENCY_NAME_LENGTH ];
    int x ;
    int y ;
    time_t timestamp ;
    }emergency_request_t ;
    typedef struct {
    emergency_type_t type ;
    emergency_status_t status ;
    int x ;
    int y ;
    time_t time ;
    int rescuer_count ;
    int id;
    rescuer_digital_twin_t** rescuers_dt ;
}emergency_t ;
typedef struct {
    emergency_t *em;
    rescuer_digital_twin_t *dt;
    int nres;
    mtx_t *mutex_log;
    emergency_t *emergenze_attive;
    int *num_emergenze_attive;
    volatile sig_atomic_t *interrompi;
    mtx_t *mutex_emergenze;
    mtx_t *mutex_rescuers; 
} gestore_args_t;
int gestisci_emergenza(void *arg);
int distanza_manhattan(int x1, int y1, int x2, int y2);
int preempt_soccorritori(emergency_t *em_new, emergency_t **attive, int num_attive, mtx_t *mutex_log);
int assegna_soccorritori(emergency_t *em, rescuer_digital_twin_t *dt, int nres, mtx_t *mutex_log, time_t timeout, mtx_t *mutex_rescuers) ;
void aging_emergenze(emergency_t *emergenze, int num, mtx_t *mutex_log,mtx_t *mutex_emergenze);
#endif
