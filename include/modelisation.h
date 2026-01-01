#ifndef MODELISATION_H
#define MODELISATION_H

#include "common.h"

typedef enum {
    ETAGE_BUREAUX = 0,
    ETAGE_REUNION,
    ETAGE_CAFETERIA,
    ETAGE_ENTREE
} TypeEtage;

typedef struct {
    int numero;        // 0..NB_ETAGES
    TypeEtage type;    // bureaux / réunion / cafétéria / entrée
} Etage;

typedef struct {
    Etage etages[NB_ETAGES + 1]; // inclut RDC
} Immeuble;

// Initialise l'immeuble (types d'étages, etc.)
void immeuble_init(Immeuble *b);

// Initialise les ascenseurs à l'état de départ
void ascenseurs_init(Ascenseur asc[NB_ASCENSEURS], int etage_depart);

// Helpers (pratiques partout)
int etage_valide(int e);
Direction direction_entre(int from, int to);
int distance_etages(int a, int b);

// Pour debug/logs
const char* type_etage_str(TypeEtage t);

#endif
