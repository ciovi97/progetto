gestisci_emergenza(*args)
{
    char id[20];
    char msg[256];
    thrd_t tid = thrd_current();
    time_t timeout;
    struct timespec ts = { .tv_sec = 1, .tv_nsec = 0 };
    uintptr_t tid_num = (uintptr_t)tid; // cast sicuro a intero che contiene un puntatore
    unsigned long id_int = (unsigned long)tid_num; // converto in unsigned long
    snprintf(id, sizeof(id), "%lu", id_int+time(NULL)); // ID univoco basato su timestamp e thread
    gestore_args_t* gargs= (gestore_args_t *)args;
    if (!gargs) {
        snprintf(msg, sizeof(msg), "Errore: argomenti nulli nella gestione dell'emergenza\n");
        scrivi_log(time(NULL), gargs->mutex_log, id, "ERRORE", msg);
        return -1; // Errore nella gestione dell'emergenza
    }
    snprintf(msg, sizeof(msg), "AVVIO EMERGENZA %s in [%d,%d]",
             gargs->em->type.emergency_desc, gargs->em->x, gargs->em->y);
    scrivi_log(time(NULL), gargs->mutex_log, id, "EMERGENCY_STATUS", msg); 
    switch(gargs->em->type->priority)
    {
        case 2:
        timeout = time(NULL) + 10;
        break;
        case 1:
        timeout=time(NULL)+ 60;
        break;
        case 0:
        timeout = time(NULL) + gargs->em->aging_time;
        break;
    }
    gargs->em->timestamp = time(NULL);
    while (true&&!*(gargs->interrompi)) {
    if(difftime(timeout,time(NULL))<=0&&(gargs->em->status==PAUSED||gargs->em->status==WAITING))
    {
        if(gargs->em->type->priority!=0)//Aging o Timeout e Cancellazione emergenza
        {
        mutex_lock(gargs->mutex_em);
        gargs->em->status = TIMEOUT;
        if(gargs->em->rescuer_count > 0)
        {
            free(gargs->em->rescuers_dt); 
        }
        mutex_unlock(gargs->mutex_em);
        snprintf(msg, sizeof(msg), "EMERGENZA %s IN [%d,%d] SCADUTA",gargs->em->type.emergency_desc, gargs->em->x, gargs->em->y);
        scrivi_log(time(NULL), gargs->mutex_log, id, "EMERGENCY_STATUS", msg);
        thrd_sleep(&ts, NULL); // Sleep to avoid busy waiting
        mutex_lock(gargs->mutex_em);
        gargs->em->status = CANCELED;
        mutex_unlock(gargs->mutex_em);
        snprintf(msg, sizeof(msg), "EMERGENZA %s IN [%d,%d] CANCELLATA",
                 gargs->em->type.emergency_desc, gargs->em->x, gargs->em->y);
        scrivi_log(time(NULL), gargs->mutex_log, id, "EMERGENCY_STATUS", msg);
        break;
        }
        else
        {
            mutex_lock(gargs->mutex_em);
            gargs->em->type->priority = 1; // Aging
            mutex_unlock(gargs->mutex_em);
            timeout= time(NULL) + 30;
            snprintf(msg, sizeof(msg), "EMERGENZA %s IN [%d,%d] AUMENTO PRIORITA'",
                 gargs->em->type.emergency_desc, gargs->em->x, gargs->em->y);
        scrivi_log(time(NULL), gargs->mutex_log, id, "AGING", msg);   
        }
    }    
        if(gargs->em->status==WAITING||gargs->em->status==PAUSED)
        {
            if(assegna_soccorritori(gargs->em, gargs->dt, gargs->nres, gargs->mutex_log, gargs->mutex_rescuers))
            {
                snprintf(msg, sizeof(msg), "EMERGENZA %s IN [%d,%d] ASSEGNATI SOCCORRITORI",gargs->em->type.emergency_desc, gargs->em->x, gargs->em->y);
                scrivi_log(time(NULL), gargs->mutex_log, id, "EMERGENCY_STATUS", msg);
                mutex_lock(gargs->mutex_em);
                gargs->em->status = ASSIGNED;
                mutex_unlock(gargs->mutex_em);
            }
            else if(gargs->em->type->priority!=0) //tento preempting
            {
                snprintf(msg, sizeof(msg), "EMERGENZA %s IN [%d,%d] PROVA PREEMPTING",
                         gargs->em->type.emergency_desc, gargs->em->x, gargs->em->y);
                scrivi_log(time(NULL), gargs->mutex_log, id, "PREEMPTING", msg);
                if(tenta_preempting(gargs->em, gargs->dt, gargs->nres, gargs->mutex_log, gargs->mutex_rescuers))
                {
                snprintf(msg, sizeof(msg), "EMERGENZA %s IN [%d,%d] PREEMPTING EFFETTUATO",
                         gargs->em->type.emergency_desc, gargs->em->x, gargs->em->y);
                scrivi_log(time(NULL), gargs->mutex_log, id, "PREEMPTING", msg);
                mutex_lock(gargs->mutex_em);
                gargs->em->status = ASSIGNED;
                mutex_unlock(gargs->mutex_em);
                }
                else
                {
                    snprintf(msg, sizeof(msg), "EMERGENZA %s IN [%d,%d] PREEMPTING NON RIUSCITO",
                         gargs->em->type.emergency_desc, gargs->em->x, gargs->em->y);
                    scrivi_log(time(NULL), gargs->mutex_log, id, "EMERGENCY_STATUS", msg);
                    continue;
                }
            }
            else
            {
                //l'emergenza è di priorità 0,non può fare preempting
                continue;
            }
        }
        if(gargs->em->status==ASSIGNED)
        {
            snprintf(msg, sizeof(msg), "SOCCORRITORI IN ARRIVO PER EMERGENZA %s IN [%d,%d]",
                 gargs->em->type.emergency_desc, gargs->em->x, gargs->em->y);
            scrivi_log(time(NULL), gargs->mutex_log, id, "EMERGENCY_STATUS", msg);
        // Aspetta che arrivino i soccorritori e nel frattempo controlla se hai subito prempting 
        //Controlla se l'emergenza è stata messa in pausa o è andato tutto liscio
            if(gargs->em->status!=PAUSED)
            {
            snprintf(msg, sizeof(msg), "SOCCORRITORI ARRIVATI PER EMERGENZA %s IN [%d,%d]",
                     gargs->em->type.emergency_desc, gargs->em->x, gargs->em->y);
            scrivi_log(time(NULL), gargs->mutex_log, id, "EMERGENCY_STATUS", msg);
            mutex_lock(gargs->mutex_em);
            gargs->em->status = IN_PROGRESS;
            mutex_unlock(gargs->mutex_em);
            }
            else
            {
                //emergenza ha subito preempting
                continue;
            }
        //aspetta che i soccorritori finiscano il lavoro e nel frattempo controlla se hai subito prempting
        }
        if(em->status==IN_PROGRESS)
        {
        //se sei arrivato qui, significe che i soccorritori hanno finito il lavoro
        mutex_lock(gargs->mutex_em);
        gargs->em->status = COMPLETED;
        mutex_unlock(gargs->mutex_em);
        snprintf(msg, sizeof(msg), "EMERGENZA %s IN [%d,%d] GESTITA CON SUCCESSO",
                 gargs->em->type.emergency_desc, gargs->em->x, gargs->em->y);
        scrivi_log(time(NULL), gargs->mutex_log, id, "EMERGENCY_STATUS", msg);
        thrd_sleep(&ts, NULL);
        snprintf(msg, sizeof(msg), "EMERGENZA %s IN [%d,%d] CANCELLATA",
                 gargs->em->type.emergency_desc, gargs->em->x, gargs->em->y);
        scrivi_log(time(NULL), gargs->mutex_log, id, "EMERGENCY_STATUS", msg);
        mutex_lock(gargs->mutex_em);
        gargs->em->status = CANCELED;
        mutex_unlock(gargs->mutex_em);
        break;
        }
        else
        {
            //emergenza è PAUSED
            continue;
        } 
    }   
    if(gargs->em->status!=CANCELED&&gargs->interrompi)
    {
        mutex_lock(gargs->mutex_em);
        gargs->em->status = CANCELED;
        gargs->em->rescuer_count = 0; // Reset rescuers count on cancel
        gargs->em->rescuers_dt = NULL; // Reset rescuers digital twins
        snprintf(msg, sizeof(msg), "EMERGENZA %s IN [%d,%d] INTERROTTA DA INTERRUPT",
                 gargs->em->type.emergency_desc, gargs->em->x, gargs->em->y);
        mutex_unlock(gargs->mutex_em);
        scrivi_log(time(NULL), gargs->mutex_log, id, "EMERGENCY_STATUS", msg);
    }
    free(gargs);
    return 0;
} 
