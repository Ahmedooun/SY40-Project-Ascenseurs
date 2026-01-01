#ifndef PANNES_H
#define PANNES_H

#include "ipc.h"
#include "common.h"
#include <pthread.h>

typedef struct {
    const IPC *ipc;
    Ascenseur *asc;
    pthread_mutex_t *asc_mtx;
    unsigned int seed;
    int min_s;
    int max_s;
    int duree_panne_s;
    int stop;
} PannesArgs;

void* pannes_thread(void *arg);

#endif
