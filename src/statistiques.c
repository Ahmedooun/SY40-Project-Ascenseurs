#include "statistiques.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

uint64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

static void ensure_cap(Stats *s, int id) {
    if (id < s->cap) return;

    int new_cap = s->cap;
    while (new_cap <= id) new_cap *= 2;

    s->creation_ms = (uint64_t*)realloc(s->creation_ms, (size_t)new_cap * sizeof(uint64_t));
    s->dropoff_ms  = (uint64_t*)realloc(s->dropoff_ms,  (size_t)new_cap * sizeof(uint64_t));

    for (int i = s->cap; i < new_cap; i++) {
        s->creation_ms[i] = 0;
        s->dropoff_ms[i] = 0;
    }
    s->cap = new_cap;
}

void stats_init(Stats *s) {
    memset(s, 0, sizeof(*s));
    s->cap = 256;
    s->creation_ms = (uint64_t*)calloc((size_t)s->cap, sizeof(uint64_t));
    s->dropoff_ms  = (uint64_t*)calloc((size_t)s->cap, sizeof(uint64_t));
}

void stats_free(Stats *s) {
    free(s->creation_ms);
    free(s->dropoff_ms);
    memset(s, 0, sizeof(*s));
}

void stats_on_demande_creee(Stats *s, const Demande *d) {
    if (!s || !d) return;
    ensure_cap(s, d->id);
    s->demandes_creees++;

    if (s->creation_ms[d->id] == 0) {
        s->creation_ms[d->id] = d->t_ms;
    }
}

void stats_on_refus(Stats *s, const Demande *d) {
    (void)d;
    if (!s) return;
    s->refus++;
}

void stats_on_dropoff(Stats *s, const Demande *d, uint64_t dropoff_time_ms) {
    if (!s || !d) return;
    ensure_cap(s, d->id);

    uint64_t c = s->creation_ms[d->id];
    if (c == 0) c = d->t_ms;

    s->dropoff_ms[d->id] = dropoff_time_ms;
    s->demandes_terminees++;

    uint64_t total = (dropoff_time_ms > c) ? (dropoff_time_ms - c) : 0;
    s->sum_total_ms += total;
    if (total > s->max_total_ms) s->max_total_ms = total;
}

void stats_on_panne(Stats *s, int asc_id) {
    (void)asc_id;
    if (!s) return;
    s->pannes++;
}

void stats_on_repare(Stats *s, int asc_id) {
    (void)asc_id;
    if (!s) return;
    s->reparations++;
}

int stats_write_csv(const Stats *s, const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return -1;

    fprintf(f, "id,creation_ms,dropoff_ms,total_ms\n");

    for (int id = 0; id < s->cap; id++) {
        if (s->creation_ms[id] == 0) continue;
        uint64_t c = s->creation_ms[id];
        uint64_t d = s->dropoff_ms[id];
        uint64_t total = (d > c) ? (d - c) : 0;

        fprintf(f, "%d,%llu,%llu,%llu\n",
                id,
                (unsigned long long)c,
                (unsigned long long)d,
                (unsigned long long)total);
    }

    fclose(f);
    return 0;
}

void stats_print_resume(const Stats *s) {
    printf("\n===== STATS =====\n");
    printf("Demandes créées     : %llu\n", (unsigned long long)s->demandes_creees);
    printf("Demandes terminées  : %llu\n", (unsigned long long)s->demandes_terminees);
    printf("Refus (requeue)     : %llu\n", (unsigned long long)s->refus);
    printf("Pannes              : %llu\n", (unsigned long long)s->pannes);
    printf("Réparations         : %llu\n", (unsigned long long)s->reparations);

    if (s->demandes_terminees > 0) {
        double avg = (double)s->sum_total_ms / (double)s->demandes_terminees;
        printf("Temps total moyen   : %.2f ms\n", avg);
        printf("Temps total max     : %llu ms\n", (unsigned long long)s->max_total_ms);
    } else {
        printf("Temps total moyen   : N/A\n");
        printf("Temps total max     : N/A\n");
    }
    printf("Fichier CSV         : stats.csv\n");
    printf("=================\n\n");
}
