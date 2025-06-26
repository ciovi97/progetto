#ifndef PARSE_ENV_H
#define PARSE_ENV_H
#include "log.h"
typedef struct 
{
    char* queue;
    int height;
    int width;
}env_t;
int carica_env(env_t* env, mtx_t* mutex);
#endif