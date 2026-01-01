#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <sys/types.h>

#define NB_ETAGES 10
#define NB_ASCENSEURS 2

typedef enum {
    DIR_UP = 1,
    DIR_DOWN = -1,
    DIR_IDLE = 0
} Direction;

typedef enum {
    ASC_IDLE = 0,
    ASC_MOVING,
    ASC_DOOR_OPEN,
    ASC_OUT_OF_SERVICE
} EtatAscenseur;

typedef enum {
    PRIO_NORMALE = 0,
    PRIO_URGENTE = 1
} Priorite;

typedef struct {
    int id;              // id demande
    int from;            // étage d'appel
    int to;              // étage destination
    Priorite prio;       // priorité
    pid_t pid_usager;    // optionnel (debug)
    uint64_t t_ms;       // optionnel (stats)
} Demande;

typedef struct {
    int id;
    int etage;
    Direction dir;
    EtatAscenseur etat;
    int charge;          // optionnel
} Ascenseur;

typedef enum {
    MSG_MISSION = 1,
    MSG_EVENT   = 2,
    MSG_STOP    = 3,
    MSG_PANNE   = 4,
    MSG_REPARE  = 5
} TypeMsg;

typedef enum {
    EVT_PICKUP = 1,
    EVT_DROPOFF,
    EVT_STATUS,
    EVT_PANNE,
    EVT_REPARE,
    EVT_REFUS      // V2: refus d’une mission (ascenseur HS)
} TypeEvent;

typedef struct {
    TypeMsg type;
    int asc_id;
    union {
        Demande mission; // pour MSG_MISSION

        struct {         // pour MSG_EVENT
            TypeEvent evt;
            int etage;       // utile pour dropoff/pickup, -1 sinon
            Demande demande; // V2: pour EVT_REFUS, on renvoie la demande complète
            uint64_t t_ms;   // optionnel
        } event;

    } data;
} MessageIPC;

#endif
