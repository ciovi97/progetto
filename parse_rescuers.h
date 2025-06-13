#ifndef PARSE_RESCUERS_H
#define PARSE_RESCUERS_H 
#include "parse_env.h"
#include "parse_rescuers.h"
#define FILENAME_R "rescuer.conf"
typedef enum {
    IDLE , EN_ROUTE_TO_SCENE , ON_SCENE , RETURNING_TO_BASE
    } rescuer_status_t ;
    typedef struct {
    char * rescuer_type_name ;
    int speed ;
    int bx;
    int by;
    } rescuer_type_t ;
typedef struct {
    int id ;
    int x ;
    int y ;
    rescuer_type_t * rescuer ;
    rescuer_status_t status ;
} rescuer_digital_twin_t;
int carica_rescuers(rescuer_digital_twin_t *dt, env_t env, rescuer_type_t *t, int *unità);
void modifica_stato(rescuer_digital_twin_t *rescuer, rescuer_status_t status);
void aggiorna_posizione(rescuer_digital_twin_t *dt,int nx, int ny);
rescuer_digital_twin_t *get_rescuer(rescuer_digital_twin_t *dt, int id,int nres);
int res_disponibili(rescuer_digital_twin_t *dt,char* ruolo,int nres);
//int assegna_soccorritori(emergency_type_t *em, rescuer_digital_twin_t *dt, int nres);
#endif