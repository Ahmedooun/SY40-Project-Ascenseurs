#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <sys/types.h>

#define NB_ETAGES 10
#define NB_ASCENSEURS 2
#define LOG_LEN 128

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
    int id;
    int from;
    int to;
    Priorite prio;
    pid_t pid_usager;
    uint64_t t_ms;
} Demande;

typedef struct {
    int id;
    int etage;
    Direction dir;
    EtatAscenseur etat;
    int charge;
} Ascenseur;

typedef enum {
    MSG_MISSION = 1,
    MSG_EVENT   = 2,
    MSG_STOP    = 3,
    MSG_PANNE   = 4,
    MSG_REPARE  = 5
} TypeMsg;

typedef enum {
    EVT_READY = 0,
    EVT_PICKUP = 1,
    EVT_DROPOFF = 2,
    EVT_STATUS = 3,
    EVT_PANNE = 4,
    EVT_REPARE = 5,
    EVT_REFUS = 6,
    EVT_LOG = 7          // ✅ nouveau : log centralisé
} TypeEvent;

typedef struct {
    TypeMsg type;
    int asc_id;
    union {
        Demande mission;

        struct {
            TypeEvent evt;
            int etage;
            Demande demande;
            uint64_t t_ms;
            char log[LOG_LEN]; // ✅ utilisé seulement si EVT_LOG
        } event;

    } data;
} MessageIPC;

#endif
