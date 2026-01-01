#include "planification.h"

static int abs_i(int x) { return x < 0 ? -x : x; }

static int score_ascenseur(const Ascenseur *a, const Demande *d) {
    if (a->etat == ASC_OUT_OF_SERVICE) return 1000000000;

    int dist = abs_i(a->etage - d->from);

    int target_dir = (d->from > a->etage) ? DIR_UP : (d->from < a->etage ? DIR_DOWN : DIR_IDLE);
    int penalty_dir = 0;
    if (a->dir != DIR_IDLE && target_dir != DIR_IDLE && a->dir != target_dir) {
        penalty_dir = 4;
    }

    int penalty_busy = (a->etat == ASC_MOVING) ? 2 : 0;
    int score = dist + penalty_dir + penalty_busy;

    if (d->prio == PRIO_URGENTE) score -= 1;

    return score;
}

int choisir_ascenseur(const Ascenseur asc[NB_ASCENSEURS], const Demande *d) {
    int best = 0;
    int best_score = score_ascenseur(&asc[0], d);

    for (int i = 1; i < NB_ASCENSEURS; i++) {
        int s = score_ascenseur(&asc[i], d);
        if (s < best_score) {
            best_score = s;
            best = i;
        }
    }
    return best;
}
