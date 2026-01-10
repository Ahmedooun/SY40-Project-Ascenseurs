#ifndef USAGERS_H
#define USAGERS_H

#include <pthread.h>
#include "common.h"

typedef struct {
    Demande *buffer;
    int cap;
    int head, tail, count;

    int stop;
    int next_id;

    pthread_mutex_t mtx;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} FileDemandes;

typedef struct {
    FileDemandes *q;
    unsigned int seed;
    int rythme_ms;
    int max_demandes;
} UsagersArgs;

int  filedemandes_init(FileDemandes *q, int cap);
void filedemandes_destroy(FileDemandes *q);

int  filedemandes_push(FileDemandes *q, const Demande *d);
int  filedemandes_pop(FileDemandes *q, Demande *out);

// stop propre de la file (débloque pop/push + stop usagers)
void filedemandes_stop(FileDemandes *q);

void* usagers_thread(void *arg);

#endif
