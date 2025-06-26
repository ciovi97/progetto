#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "parse_rescuers.h"
#include "parse_emergency_type.h"
#include "parse_env.h"
#define MAX_TIPES 6
#define FILENAME_R "rescuer.conf"
#define SCALL_N(r, c, e) do { if((r = c) == NULL) { perror(e); exit(EXIT_FAILURE); } } while(0)        
int carica_rescuers(rescuer_digital_twin_t *dt, env_t* env,rescuer_type_t *t,int *unità,mtx_t *mutex)
{
    FILE*fp;
    SCALL_N(fp,fopen(FILENAME_R, "r"),"Errore apertura /n");
    int ctipi = 0;
    int tot   = 0;
    char* nome;
    int campi[5];
    char line[256];
    char msg[300];
    while (fgets(line, sizeof(line), fp) != NULL&&ctipi<MAX_TIPES) {
    if (line[0]!= '\n' && line[0] != '#')
    {
        snprintf(msg, sizeof(msg), "Riga letta: %s", line);
        scrivi_log(time(NULL),mutex, "rescuers.conf", "PARSING", msg);
        // Parsing: [nome][numero][velocita][x;y]
        nome= strtok(line, "[]");
        campi[1] = atoi(strtok(NULL, "[]"));//numero
        campi[2] = atoi(strtok(NULL, "[]"));//velocità
        campi[3]= atoi(strtok(NULL, "[;"));//x
        campi[4]= atoi(strtok(NULL, "]"));//y
        //printf("%s,%d,%d,%d,%d\n",nome,campi[1],campi[2],campi[3],campi[4]);
    }
    if (!campi[4] || !campi[3]||campi[3] < 0 || campi[4] < 0 || campi[4] >= env->height || campi[3] >= env->height) 
        {
            snprintf(msg, sizeof(msg), "Base non valida: [%d,%d]", campi[3],campi[4]);
            scrivi_log(time(NULL),mutex, "N/A", "PARSING", msg);            
            continue;
        }
    else  if (!nome) {
            snprintf(msg, sizeof(msg), "Errore nome non valido: %s", nome);
            scrivi_log(time(NULL),mutex, "rescuers.conf", "PARSING", msg);
            continue;        
    }
    else if(!campi[1]|| campi[1]<=0 || !campi[2] ||campi[2]<=0)
    {
        snprintf(msg, sizeof(msg), "Errore velocità e/o numero unità: %d,%d", campi[1],campi[2]);
        scrivi_log(time(NULL),mutex, "rescuers.conf", "PARSING", msg);
        continue;
    }
    else{
        // campi ottenuti in ordine di celle:
        // nome
        // numero (stringa)
        // velocita (stringa)
        // "x;y"
        // salvataggio tipo di soccorritore
        t[ctipi].rescuer_type_name=malloc(strlen(nome)+1);
        strcpy(t[ctipi].rescuer_type_name, nome);
        if (!t[ctipi].rescuer_type_name) {
            perror("Errore nel nome del tipo di soccorritore\n");
            exit(EXIT_FAILURE);
        }
        t[ctipi].speed = campi[2];
        t[ctipi].bx    = campi[3];
        t[ctipi].by    = campi[4];
        unità[ctipi]   = campi[1];
        snprintf(msg, sizeof(msg),"Registrazione %d unità di %s con velocità %d e base in [%d,%d]", campi[1],t[ctipi].rescuer_type_name,t[ctipi].speed,
               t[ctipi].bx, t[ctipi].by);
        scrivi_log(time(NULL),mutex, "rescuers.conf", "PARSING", msg);
        // instanzio i digital twin di questo tipo
        for (int i = 0; i < campi[1]; i++) {
            dt[tot + i].id      = tot + i;
            dt[tot + i].x       = campi[3];
            dt[tot + i].y       = campi[4];
            dt[tot + i].rescuer = &t[ctipi];
            dt[tot + i].status  = IDLE;
        }
        tot += campi[1];
        ctipi++;
    }
}
    fclose(fp);
    if(tot>0)
    {
    snprintf(msg, sizeof(msg), "PARSING RESCUERS COMPLETATO! Caricate %d tipologie di soccorritori", tot);
    scrivi_log(time(NULL), mutex, "rescuers.conf", "PARSING", msg);
    return tot;
    }
    else
    {
        snprintf(msg, sizeof(msg), "PARSING RESCUERS COMPLETATO! Nessun soccorritore caricato");
        scrivi_log(time(NULL), mutex, "rescuers.conf", "PARSING", msg);
        return EXIT_FAILURE; // Nessun soccorritore caricato
    }

}

void modifica_stato(rescuer_digital_twin_t *rescuer, rescuer_status_t status) {
    rescuer->status = status;
}
void aggiorna_posizione(rescuer_digital_twin_t *rescuer,int nx, int ny) {
    rescuer->x = nx;
    rescuer->y = ny;
}
rescuer_digital_twin_t *get_rescuer(rescuer_digital_twin_t *dt, int id,int nres) {
    for (int i = 0; i < nres; i++) {
        if (dt[i].id == id) {
            return &dt[i];
        }
    }
    return NULL; // Non trovato
}    
int res_disponibili(rescuer_digital_twin_t *dt,char* ruolo,int n) {
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (dt[i].status==IDLE&&strcmp(dt[i].rescuer->rescuer_type_name,ruolo)==0) {
            count++;
        }
        printf("Soccorritore %d: tipo %s, posizione (%d, %d), stato IDLE\n",
                   dt[i].id,
                   dt[i].rescuer->rescuer_type_name,
                   dt[i].x,
                   dt[i].y);
    }
    return count;
}