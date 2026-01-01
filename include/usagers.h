#ifndef USAGERS_H
#define USAGERS_H

#include "common.h"
#include <pthread.h>

typedef struct {
    Demande *buffer;
    int cap;
    int head, tail, count;

    pthread_mutex_t mtx;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;

    int stop;
    int next_id;
} FileDemandes;

int  filedemandes_init(FileDemandes *q, int cap);
void filedemandes_destroy(FileDemandes *q);

int filedemandes_push(FileDemandes *q, const Demande *d);
int filedemandes_pop(FileDemandes *q, Demande *out);

typedef struct {
    FileDemandes *q;
    unsigned int seed;
    int rythme_ms;
} UsagersArgs;

void* usagers_thread(void *arg);

#endif
