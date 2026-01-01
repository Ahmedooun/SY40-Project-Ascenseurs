#include "modelisation.h"

static int abs_i(int x) { return x < 0 ? -x : x; }

void immeuble_init(Immeuble *b) {
    if (!b) return;

    for (int i = 0; i <= NB_ETAGES; i++) {
        b->etages[i].numero = i;

        if (i == 0) {
            b->etages[i].type = ETAGE_ENTREE;
        } else if (i == 5) {
            // exemple : cafétéria à l'étage 5
            b->etages[i].type = ETAGE_CAFETERIA;
        } else if (i == 8) {
            // exemple : salles de réunion à l'étage 8
            b->etages[i].type = ETAGE_REUNION;
        } else {
            b->etages[i].type = ETAGE_BUREAUX;
        }
    }
}

void ascenseurs_init(Ascenseur asc[NB_ASCENSEURS], int etage_depart) {
    for (int i = 0; i < NB_ASCENSEURS; i++) {
        asc[i].id = i;
        asc[i].etage = etage_depart;
        asc[i].dir = DIR_IDLE;
        asc[i].etat = ASC_IDLE;
        asc[i].charge = 0;
    }
}

int etage_valide(int e) {
    return (e >= 0 && e <= NB_ETAGES);
}

Direction direction_entre(int from, int to) {
    if (to > from) return DIR_UP;
    if (to < from) return DIR_DOWN;
    return DIR_IDLE;
}

int distance_etages(int a, int b) {
    return abs_i(a - b);
}

const char* type_etage_str(TypeEtage t) {
    switch (t) {
    case ETAGE_BUREAUX:   return "Bureaux";
    case ETAGE_REUNION:   return "Reunion";
    case ETAGE_CAFETERIA: return "Cafeteria";
    case ETAGE_ENTREE:    return "Entree";
    default:              return "Inconnu";
    }
}
