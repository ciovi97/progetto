#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#define MAX_TIPES 10
#define MAX_R 4
#define FILENAME_E "emergency_type.conf"
#include "parse_rescuers.h"
#include "parse_emergency_type.h"
#include "parse_env.h"
#include "log.h"
#define SCALL_N(r, c, e) do { if((r = c) == NULL) { perror(e); exit(EXIT_FAILURE); } } while(0)
rescuer_type_t *find_rescuer_type(rescuer_type_t *types,char *name, int ntypes) {
    for (int i = 0; i < ntypes; i++) {
        if (strcmp(types[i].rescuer_type_name, name) == 0)
        {
            return &types[i];
        }    
    }
    return NULL;
}
int carica_em(emergency_type_t *t, rescuer_request_t *re, rescuer_type_t *rt, int ntypes,mtx_t *mutex)
{
    FILE* fp;
    SCALL_N(fp, fopen(FILENAME_E, "r"), "Errore apertura file di emergenze");
    char line[256];
    char msg[300];
    char* nome;
    int etipi=0;;
    short pri;
    char* blocco;
    int nres;
    while (fgets(line, sizeof(line), fp) != NULL&&etipi<MAX_TIPES) {
    nres=0;
    if (line[0]!= '\n' && line[0] != '#')
    {
        snprintf(msg, sizeof(msg), "Riga letta: %s", line);
        scrivi_log(time(NULL),mutex, "emergency_type.conf", "PARSING", msg);
          /* sostituisco le parentesi quadre con spazi per usare strtok */
        for (char *p = line; *p; ++p) {
            if (*p == '[' || *p == ']') {
                *p = ' ';
            }
        }
        // Parsing: [nome][numero][velocita][x;y]
        nome=strtok(line," \t\n");
        pri= atoi(strtok(NULL," \t\n"));//priorità
        if(!nome || pri<0||pri>2) {
        snprintf(msg, sizeof(msg), "Riga malformata");
        scrivi_log(time(NULL),mutex, "emergency_type.conf", "PARSING", msg);
            continue; 
        }
        t[etipi].emergency_desc=malloc(strlen(nome)+1);
        if (!t[etipi].emergency_desc) {
            perror("Errore allocazione memoria per descrizione emergenza");
            exit(EXIT_FAILURE);
        }
        snprintf(msg, sizeof(msg), "Tipo di emergenza: %s con priorità: %d. Richiede:", nome, pri);
        scrivi_log(time(NULL),mutex, "emergency_type.conf", "PARSING", msg);
        strcpy(t[etipi].emergency_desc, nome);
        t[etipi].priority = pri;
        blocco=strtok(NULL,"");
        while(blocco) {
            if(blocco[0]=='\n' || blocco[0]=='\0') {
                continue; // salta spazi vuoti
            }
            while(blocco &&(isspace((unsigned char)*blocco)))
            {
                blocco++; // salta spazi iniziali
            }
        re[nres].type=find_rescuer_type(rt,strtok(blocco, ":"),ntypes); // numero richiesto
        re[nres].required_count=atoi(strtok(NULL, ",")); // numero richiesto
        re[nres].time_to_manage=atoi(strtok(NULL, ";")); // tempo di gestione
        snprintf(msg, sizeof(msg), "- %d unità di %s che impiegano %d secondi per l'emergenza",re[nres].required_count, re[nres].type->rescuer_type_name, re[nres].time_to_manage);
        scrivi_log(time(NULL),mutex, "", "", msg);
        nres++;
        blocco=strtok(NULL, ";");     
        }
        t[etipi].rescuers = malloc(sizeof(rescuer_request_t) * nres);
        if (!t[etipi].rescuers) {
            perror("Errore allocazione memoria per richieste soccorritori");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < nres; i++) {
                    t[etipi].rescuers[i] = re[i];
        }
        t[etipi].rescuers_req_number = nres;
        etipi++;
    }
}
fclose(fp);
snprintf(msg, sizeof(msg), "PARSING EMERGENZE COMPLETATO! Caricate %d tipologie di emergenze", etipi);
scrivi_log(time(NULL), mutex, "emergency_type.conf", "PARSING", msg);
return 0;
}