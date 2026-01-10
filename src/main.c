#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#include "common.h"
#include "ipc.h"
#include "ascenseur.h"
#include "planification.h"
#include "usagers.h"
#include "pannes.h"
#include "modelisation.h"
#include "statistiques.h"
#include "interface.h"

#define NB_MISSIONS_AUTO 30

static int choisir_mode(void) {
    int mode = 0;
    printf("\n=== Choix de simulation ===\n");
    printf("1) Automatique (demandes auto, appels manuels désactivés)\n");
    printf("2) Manuelle   (aucune demande auto, appels manuels uniquement)\n");
    printf("Votre choix (1/2) : ");
    fflush(stdout);

    while (mode != 1 && mode != 2) {
        if (scanf("%d", &mode) != 1) {
            int c; while ((c = getchar()) != '\n' && c != EOF) {}
            mode = 0;
        } else {
            int c; while ((c = getchar()) != '\n' && c != EOF) {}
        }
        if (mode != 1 && mode != 2) {
            printf("Tape 1 ou 2 : ");
            fflush(stdout);
        }
    }

    printf("Mode sélectionné : %s\n\n", (mode == 1) ? "AUTOMATIQUE" : "MANUEL");
    return mode;
}

static void attendre_ready(const IPC *ipc) {
    int ready[NB_ASCENSEURS] = {0};
    int nb_ready = 0;

    while (nb_ready < NB_ASCENSEURS) {
        MessageIPC ev;
        int r = ipc_recv_event(ipc, &ev, 1); // bloquant
        if (r != 0) continue;
        if (ev.type != MSG_EVENT) continue;

        if (ev.data.event.evt == EVT_LOG) {
            printf("%s\n", ev.data.event.log);
            continue;
        }

        if (ev.data.event.evt == EVT_READY) {
            int id = ev.asc_id;
            if (id >= 0 && id < NB_ASCENSEURS && !ready[id]) {
                ready[id] = 1;
                nb_ready++;
            }
        }
    }

    printf("\n[CTRL] Tous les ascenseurs sont prêts.\n");
}

static int auto_plus_de_demandes_a_attendre(const FileDemandes *q, int done, int target) {
    // On considère que toutes les demandes auto ont été générées quand next_id > target
    // et qu'il n'y a plus rien en file.
    int empty = 0;
    int all_generated = 0;

    pthread_mutex_lock((pthread_mutex_t*)&q->mtx);
    empty = (q->count == 0);
    all_generated = (q->next_id > target);
    pthread_mutex_unlock((pthread_mutex_t*)&q->mtx);

    return (all_generated && empty && done < target);
}

// Traite un event unique. Retourne 1 si stop_global a été déclenché, sinon 0.
static int traiter_event(
    const MessageIPC *ev,
    int mode, int target, int *done, int *stop_global,
    Stats *stats,
    FileDemandes *q,
    PannesArgs *pargs,
    Ascenseur asc[],
    pthread_mutex_t *asc_mtx
) {
    int id = ev->asc_id;

    if (ev->data.event.evt == EVT_LOG) {
        printf("%s\n", ev->data.event.log);
        return 0;
    }
    if (ev->data.event.evt == EVT_READY) return 0;

    if (ev->data.event.evt == EVT_REFUS) {
        Demande dref = ev->data.event.demande;
        printf("[CTRL] REFUS asc=%d demande #%d %d->%d => requeue\n",
               id, dref.id, dref.from, dref.to);

        stats_on_refus(stats, &dref);
        filedemandes_push(q, &dref);
        return 0;
    }

    if (ev->data.event.evt == EVT_DROPOFF) {
        Demande dd = ev->data.event.demande;
        stats_on_dropoff(stats, &dd, ev->data.event.t_ms);

        // ✅ STRICT : on compte au max target
        if (mode == 1 && *done < target) {
            (*done)++;
            // Optionnel mais pratique :
            // printf("[CTRL] Progression: %d/%d\n", *done, target);

            if (*done == target) {
                // Stop immédiat et propre
                *stop_global = 1;
                pargs->stop = 1;         // stop pannes
                filedemandes_stop(q);    // stop usagers + débloque pop/push
                return 1;
            }
        }
    } else if (ev->data.event.evt == EVT_PANNE) {
        stats_on_panne(stats, id);
    } else if (ev->data.event.evt == EVT_REPARE) {
        stats_on_repare(stats, id);
    }

    // Mise à jour état ascenseurs (utile pour status)
    pthread_mutex_lock(asc_mtx);
    if (ev->data.event.evt == EVT_PANNE) {
        asc[id].etat = ASC_OUT_OF_SERVICE;
    } else if (ev->data.event.evt == EVT_REPARE) {
        asc[id].etat = ASC_IDLE;
        asc[id].dir = DIR_IDLE;
    } else if (ev->data.event.evt == EVT_DROPOFF) {
        asc[id].etage = ev->data.event.etage;
        if (asc[id].etat != ASC_OUT_OF_SERVICE) {
            asc[id].etat = ASC_IDLE;
            asc[id].dir  = DIR_IDLE;
        }
    }
    pthread_mutex_unlock(asc_mtx);

    return 0;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    // Seed variable : scénario différent à chaque RUN
    unsigned int base_seed = (unsigned int)time(NULL);
    printf("[CTRL] Seed simulation = %u\n", base_seed);

    IPC ipc = {.q_ctrl_to_asc = -1, .q_asc_to_ctrl = -1};
    if (ipc_init(&ipc, "ipc.key", 65, 66) != 0) {
        fprintf(stderr, "IPC init failed\n");
        return 1;
    }

    Immeuble b;
    immeuble_init(&b);

    Ascenseur asc[NB_ASCENSEURS];
    ascenseurs_init(asc, 0);

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

    attendre_ready(&ipc);

    int mode = choisir_mode();

    FileDemandes q;
    if (filedemandes_init(&q, 128) != 0) {
        fprintf(stderr, "filedemandes_init failed\n");
        ipc_cleanup(&ipc);
        return 1;
    }

    // Usagers : uniquement AUTO, et max_demandes = N
    pthread_t th_usagers;
    int usagers_lances = 0;
    UsagersArgs uargs = {
        .q = &q,
        .seed = base_seed,
        .rythme_ms = 400,
        .max_demandes = 0
    };
    if (mode == 1) {
        uargs.max_demandes = NB_MISSIONS_AUTO;
        pthread_create(&th_usagers, NULL, usagers_thread, &uargs);
        usagers_lances = 1;
    }

    // Pannes (deux modes), seed dérivée
    pthread_t th_pannes;
    PannesArgs pargs = {
        .ipc = &ipc,
        .asc = asc,
        .asc_mtx = &asc_mtx,
        .seed = base_seed ^ 0xA5A5A5A5u,
        .min_s = 3,
        .max_s = 8,
        .duree_panne_s = 5,
        .stop = 0
    };
    pthread_create(&th_pannes, NULL, pannes_thread, &pargs);

    // Interface (version poll non bloquante)
    int stop_global = 0;
    pthread_t th_ui;
    InterfaceArgs iargs = {
        .q = &q,
        .asc = asc,
        .nb_asc = NB_ASCENSEURS,
        .asc_mtx = &asc_mtx,
        .stop_global = &stop_global,
        .mode = mode
    };
    pthread_create(&th_ui, NULL, interface_thread, &iargs);

    // STRICT : arrêt auto sur N dropoffs
    const int target = (mode == 1) ? NB_MISSIONS_AUTO : -1;
    int done = 0;

    while (!stop_global) {
        // 1) Draine events non-bloquant
        while (1) {
            MessageIPC ev;
            int r = ipc_recv_event(&ipc, &ev, 0);
            if (r == 1) break;
            if (r != 0) break;
            if (ev.type != MSG_EVENT) continue;

            if (traiter_event(&ev, mode, target, &done, &stop_global,
                              &stats, &q, &pargs, asc, &asc_mtx)) {
                break;
            }
        }
        if (stop_global) break;

        // ✅ FIX IMPORTANT :
        // En auto, une fois que toutes les demandes ont été générées et que la file est vide,
        // NE PAS bloquer sur filedemandes_pop(), sinon on ne traite plus les EVT_DROPOFF !
        if (mode == 1 && auto_plus_de_demandes_a_attendre(&q, done, target)) {
            // On attend un event (bloquant) et on le traite, jusqu’à atteindre done==target.
            MessageIPC ev;
            int r = ipc_recv_event(&ipc, &ev, 1); // bloquant
            if (r == 0 && ev.type == MSG_EVENT) {
                traiter_event(&ev, mode, target, &done, &stop_global,
                              &stats, &q, &pargs, asc, &asc_mtx);
            }
            continue; // on retourne drainer le reste
        }

        // 2) Pop demande (bloquant) — OK tant qu'il peut encore y avoir des demandes
        Demande d;
        if (filedemandes_pop(&q, &d) != 0) break;

        if (!etage_valide(d.from) || !etage_valide(d.to)) {
            printf("[CTRL] Demande invalide #%d (%d->%d) ignorée\n", d.id, d.from, d.to);
            continue;
        }

        stats_on_demande_creee(&stats, &d);

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

        MessageIPC m = {0};
        m.type = MSG_MISSION;
        m.asc_id = chosen;
        m.data.mission = d;
        ipc_send_mission(&ipc, chosen, &m);
    }

    // Arrêt propre
    pargs.stop = 1;
    pthread_join(th_pannes, NULL);

    filedemandes_stop(&q); // idempotent
    pthread_join(th_ui, NULL);

    if (usagers_lances) pthread_join(th_usagers, NULL);

    filedemandes_destroy(&q);

    // Stop ascenseurs
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

    // Stats
    stats_write_csv(&stats, "stats.csv");
    stats_print_resume(&stats);
    stats_free(&stats);

    printf("[CTRL] Fin simulation.\n");
    return 0;
}
