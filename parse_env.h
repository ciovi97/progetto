#ifndef PARSE_ENV_H
#define PARSE_ENV_H
typedef struct 
{
    char* queue;
    int height;
    int width;
}env_t;
int carica_env(char* filename,env_t* matrice);
#endif