#ifndef INTERFACE_H
#define INTERFACE_H

#include <pthread.h>
#include "usagers.h"
#include "common.h"

typedef struct {
    FileDemandes *q;

    Ascenseur *asc;
    int nb_asc;
    pthread_mutex_t *asc_mtx;

    int *stop_global;

    // 1 = auto (call désactivé), 2 = manuel (call autorisé)
    int mode;
} InterfaceArgs;

void* interface_thread(void *arg);

#endif
