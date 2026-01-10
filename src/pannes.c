#include "pannes.h"
#include "common.h"
#include "ipc.h"
#include "statistiques.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

static int rand_between(unsigned int *seed, int a, int b) {
    int r = (int)(rand_r(seed) % (unsigned)(b - a + 1));
    return a + r;
}

static void send_log(const IPC *ipc, int asc_id, const char *msg) {
    MessageIPC ev = {0};
    ev.type = MSG_EVENT;
    ev.asc_id = asc_id;
    ev.data.event.evt = EVT_LOG;
    ev.data.event.etage = -1;
    ev.data.event.t_ms = now_ms();
    snprintf(ev.data.event.log, LOG_LEN, "%s", msg);
    ipc_send_event(ipc, &ev);
}

void* pannes_thread(void *arg) {
    PannesArgs *a = (PannesArgs*)arg;

    while (!a->stop) {
        // attendre un délai aléatoire (découpé en tranches pour pouvoir stopper vite)
        int wait_s = rand_between(&a->seed, a->min_s, a->max_s);
        for (int i = 0; i < wait_s * 10 && !a->stop; i++) {
            usleep(100000); // 0.1s
        }
        if (a->stop) break;

        int id = rand_between(&a->seed, 0, NB_ASCENSEURS - 1);

        // Déclenche panne seulement si pas déjà HS
        pthread_mutex_lock(a->asc_mtx);
        if (a->asc[id].etat == ASC_OUT_OF_SERVICE) {
            pthread_mutex_unlock(a->asc_mtx);
            continue;
        }
        a->asc[id].etat = ASC_OUT_OF_SERVICE;
        pthread_mutex_unlock(a->asc_mtx);

        char buf[LOG_LEN];
        snprintf(buf, sizeof(buf), "[PANNE] Déclenchement panne sur asc %d", id);
        send_log(a->ipc, id, buf);

        MessageIPC m = {0};
        m.type = MSG_PANNE;
        m.asc_id = id;
        ipc_send_mission(a->ipc, id, &m);

        // durée de la panne (découpée)
        for (int i = 0; i < a->duree_panne_s * 10 && !a->stop; i++) {
            usleep(100000);
        }
        if (a->stop) break;

        // Réparation
        pthread_mutex_lock(a->asc_mtx);
        a->asc[id].etat = ASC_IDLE;
        a->asc[id].dir = DIR_IDLE;
        pthread_mutex_unlock(a->asc_mtx);

        snprintf(buf, sizeof(buf), "[PANNE] Réparation asc %d", id);
        send_log(a->ipc, id, buf);

        MessageIPC r = {0};
        r.type = MSG_REPARE;
        r.asc_id = id;
        ipc_send_mission(a->ipc, id, &r);
    }

    return NULL;
}
