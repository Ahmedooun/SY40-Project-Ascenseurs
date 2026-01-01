#ifndef STATISTIQUES_H
#define STATISTIQUES_H

#include <stdint.h>
#include "common.h"

uint64_t now_ms(void);

typedef struct {
    uint64_t demandes_creees;
    uint64_t demandes_terminees;
    uint64_t refus;
    uint64_t pannes;
    uint64_t reparations;

    uint64_t sum_total_ms;
    uint64_t max_total_ms;

    uint64_t *creation_ms;
    uint64_t *dropoff_ms;
    int cap;
} Stats;

void stats_init(Stats *s);
void stats_free(Stats *s);

void stats_on_demande_creee(Stats *s, const Demande *d);
void stats_on_refus(Stats *s, const Demande *d);
void stats_on_dropoff(Stats *s, const Demande *d, uint64_t dropoff_time_ms);

void stats_on_panne(Stats *s, int asc_id);
void stats_on_repare(Stats *s, int asc_id);

int  stats_write_csv(const Stats *s, const char *path);
void stats_print_resume(const Stats *s);

#endif
