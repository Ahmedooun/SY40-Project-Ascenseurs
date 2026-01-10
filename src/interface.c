#include "interface.h"
#include "statistiques.h"
#include "modelisation.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>

static void print_help(int mode) {
    printf("\n=== Commandes ===\n");
    printf("help\n");
    printf("status\n");
    if (mode == 2) {
        printf("call <from> <to> [prio]\n");
        printf("  prio: 0 = normale (defaut), 1 = urgente\n");
    } else {
        printf("call ...  (désactivé en mode AUTO)\n");
    }
    printf("quit\n");
    printf("===============\n\n");
}

static void print_status(const InterfaceArgs *a) {
    pthread_mutex_lock(a->asc_mtx);
    printf("\n--- STATUS ASCENSEURS ---\n");
    for (int i = 0; i < a->nb_asc; i++) {
        const Ascenseur *x = &a->asc[i];
        printf("Asc %d | etage=%d | etat=%d | dir=%d\n",
               x->id, x->etage, x->etat, x->dir);
    }
    printf("-------------------------\n\n");
    pthread_mutex_unlock(a->asc_mtx);
}

void* interface_thread(void *arg) {
    InterfaceArgs *a = (InterfaceArgs*)arg;
    print_help(a->mode);

    struct pollfd pfd;
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;

    char line[256];

    while (!*a->stop_global) {
        // poll avec timeout pour pouvoir sortir même sans input
        int pr = poll(&pfd, 1, 200); // 200 ms
        if (pr <= 0) continue;       // timeout ou erreur => on reboucle pour re-check stop_global

        if (pfd.revents & POLLIN) {
            if (!fgets(line, (int)sizeof(line), stdin)) {
                // EOF
                *a->stop_global = 1;
                filedemandes_stop(a->q);
                break;
            }

            line[strcspn(line, "\n")] = 0;

            if (strcmp(line, "help") == 0) {
                print_help(a->mode);
                continue;
            }

            if (strcmp(line, "status") == 0) {
                print_status(a);
                continue;
            }

            if (strcmp(line, "quit") == 0) {
                printf("[UI] Arrêt demandé.\n");
                *a->stop_global = 1;

                // ✅ crucial : débloquer le contrôleur même si file vide
                filedemandes_stop(a->q);
                break;
            }

            if (strncmp(line, "call", 4) == 0) {
                if (a->mode != 2) {
                    printf("[UI] Mode AUTO : appels manuels désactivés.\n");
                    continue;
                }

                int from = -1, to = -1, prio_i = 0;
                int n = sscanf(line, "call %d %d %d", &from, &to, &prio_i);
                if (n < 2) {
                    printf("[UI] Usage: call <from> <to> [prio]\n");
                    continue;
                }

                if (!etage_valide(from) || !etage_valide(to) || from == to) {
                    printf("[UI] Demande invalide (from=%d, to=%d)\n", from, to);
                    continue;
                }

                Priorite prio = (prio_i == 1) ? PRIO_URGENTE : PRIO_NORMALE;

                Demande d = {0};
                pthread_mutex_lock(&a->q->mtx);
                d.id = a->q->next_id++;
                pthread_mutex_unlock(&a->q->mtx);

                d.from = from;
                d.to = to;
                d.prio = prio;
                d.t_ms = now_ms();

                if (filedemandes_push(a->q, &d) == 0) {
                    printf("[UI] Demande injectée: #%d %d->%d prio=%d\n",
                           d.id, d.from, d.to, d.prio);
                } else {
                    printf("[UI] Impossible d'injecter la demande (file stoppée)\n");
                }
                continue;
            }

            printf("[UI] Commande inconnue. Tape 'help'.\n");
        }
    }

    return NULL;
}
