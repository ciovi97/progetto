#include "parse_rescuers.h"
#ifndef PARSE_EMERGENCY_TYPE_H
#define PARSE_EMERGENCY_TYPE_H
#include "parse_env.h"
typedef struct {
    rescuer_type_t *type ;
    int required_count ;
    int time_to_manage ;
} rescuer_request_t;
typedef struct {
    short priority ;
    char * emergency_desc ;
    rescuer_request_t *rescuers ;
    int rescuers_req_number ;
} emergency_type_t;
rescuer_type_t *find_rescuer_type(rescuer_type_t *types,char *name,int ntypes);
void carica_em(emergency_type_t *t, rescuer_request_t *re, rescuer_type_t *rt, int ntype);
#endif
