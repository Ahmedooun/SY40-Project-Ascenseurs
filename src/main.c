#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "common.h"
#include "ipc.h"
#include "ascenseur.h"
#include "planification.h"
#include "usagers.h"
#include "pannes.h"
#include "modelisation.h"
#include "statistiques.h"

static void stop_queue(FileDemandes *q) {
    pthread_mutex_lock(&q->mtx);
    q->stop = 1;
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->mtx);
}

int main(void) {
    IPC ipc = {.q_ctrl_to_asc = -1, .q_asc_to_ctrl = -1};

    if (ipc_init(&ipc, "ipc.key", 65, 66) != 0) {
        fprintf(stderr, "IPC init failed\n");
        return 1;
    }

    // Modélisation
    Immeuble b;
    immeuble_init(&b);

    Ascenseur asc[NB_ASCENSEURS];
    ascenseurs_init(asc, 0);

    // Stats
    Stats stats;
    stats_init(&stats);

    pthread_mutex_t asc_mtx;
    pthread_mutex_init(&asc_mtx, NULL);

    // Fork ascenseurs
    pid_t pids[NB_ASCENSEURS];
    for (int i = 0; i < NB_ASCENSEURS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            ascenseur_run(i, &ipc);
            _exit(0);
        }
        pids[i] = pid;
    }

    // File + thread usagers
    FileDemandes q;
    if (filedemandes_init(&q, 128) != 0) {
        fprintf(stderr, "filedemandes_init failed\n");
        ipc_cleanup(&ipc);
        return 1;
    }

    pthread_t th_usagers;
    UsagersArgs uargs = {.q = &q, .seed = 12345u, .rythme_ms = 400};
    pthread_create(&th_usagers, NULL, usagers_thread, &uargs);

    // Thread pannes
    pthread_t th_pannes;
    PannesArgs pargs = {
        .ipc = &ipc,
        .asc = asc,
        .asc_mtx = &asc_mtx,
        .seed = 777u,
        .min_s = 3,
        .max_s = 8,
        .duree_panne_s = 5,
        .stop = 0
    };
    pthread_create(&th_pannes, NULL, pannes_thread, &pargs);

    int a_traiter = 30;

    while (a_traiter-- > 0) {
        // Drain events non bloquant
        while (1) {
            MessageIPC ev;
            int r = ipc_recv_event(&ipc, &ev, 0);
            if (r == 1) break;
            if (r != 0) break;

            if (ev.type != MSG_EVENT) continue;

            int id = ev.asc_id;

            if (ev.data.event.evt == EVT_REFUS) {
                Demande dref = ev.data.event.demande;
                printf("[CTRL] REFUS asc=%d demande #%d %d->%d => requeue\n",
                       id, dref.id, dref.from, dref.to);

                stats_on_refus(&stats, &dref);
                filedemandes_push(&q, &dref);
                continue;
            }

            if (ev.data.event.evt == EVT_DROPOFF) {
                Demande dd = ev.data.event.demande;
                stats_on_dropoff(&stats, &dd, ev.data.event.t_ms);
            } else if (ev.data.event.evt == EVT_PANNE) {
                stats_on_panne(&stats, id);
            } else if (ev.data.event.evt == EVT_REPARE) {
                stats_on_repare(&stats, id);
            }

            pthread_mutex_lock(&asc_mtx);

            if (ev.data.event.evt == EVT_PANNE) {
                asc[id].etat = ASC_OUT_OF_SERVICE;
            } else if (ev.data.event.evt == EVT_REPARE) {
                asc[id].etat = ASC_IDLE;
                asc[id].dir = DIR_IDLE;
            } else {
                asc[id].etage = ev.data.event.etage;
                if (asc[id].etat != ASC_OUT_OF_SERVICE) {
                    asc[id].etat = ASC_IDLE;
                    asc[id].dir  = DIR_IDLE;
                }
            }

            pthread_mutex_unlock(&asc_mtx);

            printf("[CTRL] Event: asc=%d evt=%d etage=%d\n",
                   id, ev.data.event.evt, ev.data.event.etage);
        }

        // Pop demande
        Demande d;
        if (filedemandes_pop(&q, &d) != 0) break;

        if (!etage_valide(d.from) || !etage_valide(d.to)) {
            printf("[CTRL] Demande invalide #%d (%d->%d) ignorée\n", d.id, d.from, d.to);
            continue;
        }

        // Stats: enregistre création (si première fois)
        stats_on_demande_creee(&stats, &d);

        // Choix ascenseur
        int chosen;
        pthread_mutex_lock(&asc_mtx);
        chosen = choisir_ascenseur(asc, &d);

        if (asc[chosen].etat != ASC_OUT_OF_SERVICE) {
            asc[chosen].etat = ASC_MOVING;
            asc[chosen].dir = direction_entre(asc[chosen].etage, d.from);
        }
        pthread_mutex_unlock(&asc_mtx);

        printf("[CTRL] Demande #%d %d->%d prio=%d => asc %d\n",
               d.id, d.from, d.to, d.prio, chosen);

        // Envoyer mission
        MessageIPC m = {0};
        m.type = MSG_MISSION;
        m.asc_id = chosen;
        m.data.mission = d;

        ipc_send_mission(&ipc, chosen, &m);
    }

    // Arrêt propre
    stop_queue(&q);
    pthread_join(th_usagers, NULL);

    pargs.stop = 1;
    pthread_join(th_pannes, NULL);

    filedemandes_destroy(&q);

    for (int i = 0; i < NB_ASCENSEURS; i++) {
        MessageIPC stopm = {0};
        stopm.type = MSG_STOP;
        stopm.asc_id = i;
        ipc_send_mission(&ipc, i, &stopm);
    }

    for (int i = 0; i < NB_ASCENSEURS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    pthread_mutex_destroy(&asc_mtx);

    ipc_cleanup(&ipc);

    // Export stats
    stats_write_csv(&stats, "stats.csv");
    stats_print_resume(&stats);
    stats_free(&stats);

    printf("[CTRL] Fin simulation.\n");
    return 0;
}
