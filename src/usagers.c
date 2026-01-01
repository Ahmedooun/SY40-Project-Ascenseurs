#include "usagers.h"

#include <stdlib.h>
#include <unistd.h>

static int rand_between(unsigned int *seed, int a, int b) {
    int r = (int)(rand_r(seed) % (unsigned)(b - a + 1));
    return a + r;
}

int filedemandes_init(FileDemandes *q, int cap) {
    q->buffer = (Demande*)calloc((size_t)cap, sizeof(Demande));
    if (!q->buffer) return -1;

    q->cap = cap;
    q->head = q->tail = q->count = 0;
    q->stop = 0;
    q->next_id = 1;

    pthread_mutex_init(&q->mtx, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
    return 0;
}

void filedemandes_destroy(FileDemandes *q) {
    if (!q) return;
    free(q->buffer);
    pthread_mutex_destroy(&q->mtx);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
}

int filedemandes_push(FileDemandes *q, const Demande *d) {
    pthread_mutex_lock(&q->mtx);
    while (!q->stop && q->count == q->cap) pthread_cond_wait(&q->not_full, &q->mtx);
    if (q->stop) { pthread_mutex_unlock(&q->mtx); return -1; }

    q->buffer[q->tail] = *d;
    q->tail = (q->tail + 1) % q->cap;
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mtx);
    return 0;
}

int filedemandes_pop(FileDemandes *q, Demande *out) {
    pthread_mutex_lock(&q->mtx);
    while (!q->stop && q->count == 0) pthread_cond_wait(&q->not_empty, &q->mtx);
    if (q->stop) { pthread_mutex_unlock(&q->mtx); return -1; }

    *out = q->buffer[q->head];
    q->head = (q->head + 1) % q->cap;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mtx);
    return 0;
}

void* usagers_thread(void *arg) {
    UsagersArgs *a = (UsagersArgs*)arg;
    FileDemandes *q = a->q;

    while (1) {
        pthread_mutex_lock(&q->mtx);
        int stop = q->stop;
        pthread_mutex_unlock(&q->mtx);
        if (stop) break;

        Demande d = {0};

        pthread_mutex_lock(&q->mtx);
        d.id = q->next_id++;
        pthread_mutex_unlock(&q->mtx);

        d.from = rand_between(&a->seed, 0, NB_ETAGES);
        do {
            d.to = rand_between(&a->seed, 0, NB_ETAGES);
        } while (d.to == d.from);

        d.prio = (rand_between(&a->seed, 1, 20) == 1) ? PRIO_URGENTE : PRIO_NORMALE;

        filedemandes_push(q, &d);

        usleep((useconds_t)a->rythme_ms * 1000u);
    }
    return NULL;
}
