#include "pannes.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static int rand_between(unsigned int *seed, int a, int b) {
    return a + (int)(rand_r(seed) % (unsigned)(b - a + 1));
}

void* pannes_thread(void *arg) {
    PannesArgs *p = (PannesArgs*)arg;

    while (!p->stop) {
        int wait_s = rand_between(&p->seed, p->min_s, p->max_s);
        sleep(wait_s);
        if (p->stop) break;

        int id = rand_between(&p->seed, 0, NB_ASCENSEURS - 1);

        pthread_mutex_lock(p->asc_mtx);
        int deja = (p->asc[id].etat == ASC_OUT_OF_SERVICE);
        if (!deja) p->asc[id].etat = ASC_OUT_OF_SERVICE;
        pthread_mutex_unlock(p->asc_mtx);

        if (deja) continue;

        printf("[PANNE] Déclenchement panne sur asc %d\n", id);

        MessageIPC pm = {0};
        pm.type = MSG_PANNE;
        pm.asc_id = id;
        ipc_send_mission(p->ipc, id, &pm);

        sleep(p->duree_panne_s);

        pthread_mutex_lock(p->asc_mtx);
        p->asc[id].etat = ASC_IDLE;
        pthread_mutex_unlock(p->asc_mtx);

        printf("[PANNE] Réparation asc %d\n", id);

        MessageIPC rm = {0};
        rm.type = MSG_REPARE;
        rm.asc_id = id;
        ipc_send_mission(p->ipc, id, &rm);
    }

    return NULL;
}
