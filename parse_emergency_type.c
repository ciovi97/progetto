#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#ifndef PARSE_EMERGENCY_TYPE_H
#define PARSE EMERGENCY_TYPE_H
#define MAX_TIPES 10
#define MAX_R 4
#define FILENAME_E "emergency_type.conf"
#include "parse_rescuers.h"
#include "parse_emergency_type.h"
#include "parse_env.h"
#define SCALL_N(r, c, e) do { if((r = c) == NULL) { perror(e); exit(EXIT_FAILURE); } } while(0)
rescuer_type_t *find_rescuer_type(rescuer_type_t *r, rescuer_type_t *types,char *name, int ntypes) {
    for (int i = 0; i < ntypes; i++) {
        if (strcmp(types[i].rescuer_type_name, name) == 0)
        {
            return &types[i];
        }    
    }
    return NULL;
}
void carica_em(emergency_type_t *t, rescuer_request_t *re, rescuer_type_t *rt,int ntypes)
{
    FILE* fp;
    SCALL_N(fp, fopen(FILENAME_E, "r"), "Errore apertura file di emergenze");
    char line[256];
    char* nome;
    int etipi=0,n,ti,nres;
    short int pri;
    char* r;
    char* blocco;
    while (fgets(line, sizeof(line), fp) != NULL&&etipi<MAX_TIPES) {
    nres=0;
    printf("EIIIIIIIII\n");
    if (line[0]!= '\n' && line[0] != '#')
    {
        // Parsing: [nome][numero][velocita][x;y]
        nome=strtok(line, "[]");
        printf("nome: %s\n", nome);
        pri= atoi(strtok(NULL, " []"));//priorità
        printf("pri: %d\n", pri);
        if(!nome || pri<0||pri>2) {
            fprintf(stderr, "Errore nel formato della riga: %s\n", line);
            continue; 
        }
        t[etipi].emergency_desc=malloc(strlen(nome)+1);
        if (!t[etipi].emergency_desc) {
            perror("Errore allocazione memoria per descrizione emergenza");
            exit(EXIT_FAILURE);
        }
        strcpy(t[etipi].emergency_desc, nome);
        t[etipi].priority = pri;
        blocco=strtok(NULL,";");
        while(blocco) {
            if(blocco[0]=='\n' || blocco[0]=='\0') {
                continue; // salta spazi vuoti
            }
            while(blocco &&( isspace((unsigned char)*blocco)))
            {
                blocco++; // salta spazi iniziali
            }
        printf("blocco 1: %s\n", blocco);
        re[nres].type=find_rescuer_type(re[nres].type,rt,strtok(blocco, ":"),ntypes); // numero richiesto
        re[nres].required_count=atoi(strtok(NULL, ",")); // numero richiesto
        re[nres].time_to_manage=atoi(strtok(NULL, ";")); // tempo di gestione
        printf("  - %s: %d unità, tempo di gestione: %d secondi\n", 
               re[nres].type->rescuer_type_name, re[nres].required_count, re[nres].time_to_manage);
        nres++;
        printf("blocco 2: %s\n", blocco);     
        }
        printf("\n");
        t[etipi].rescuers=re;
        t[etipi].rescuers_req_number = nres+1;
        /*printf("Gestione emergenza: %s, priorità: %d, numero richiesto totale: %d di\n", 
               t[etipi].emergency_desc, t[etipi].priority, t[etipi].rescuers_req_number);
        for (int i = 0; i < nres; i++) {
            printf("  - %s: %d unità, tempo di gestione: %d secondi\n", 
                   t[etipi].rescuers[i].type->rescuer_type_name, t[etipi].rescuers[i].required_count, t[etipi].rescuers[i].time_to_manage);
        } */
        etipi++;
        blocco=strtok(NULL,";");
        printf("blocco 2: %s\n", blocco); 
    }
}
return;
}
#endif