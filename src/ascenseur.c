#include "ascenseur.h"
#include "common.h"
#include "ipc.h"
#include "statistiques.h"

#include <stdio.h>
#include <unistd.h>

void ascenseur_run(int asc_id, const IPC *ipc) {
    printf("[ASC %d] démarré (pid=%d)\n", asc_id, getpid());

    int out_of_service = 0;

    while (1) {
        MessageIPC m;
        int r = ipc_recv_mission(ipc, asc_id, &m, 1);
        if (r != 0) continue;

        if (m.type == MSG_STOP) {
            printf("[ASC %d] STOP reçu, arrêt.\n", asc_id);
            break;
        }

        if (m.type == MSG_PANNE) {
            out_of_service = 1;
            printf("[ASC %d] PANNE ! Hors service.\n", asc_id);

            MessageIPC ev = {0};
            ev.type = MSG_EVENT;
            ev.asc_id = asc_id;
            ev.data.event.evt = EVT_PANNE;
            ev.data.event.etage = -1;
            ev.data.event.t_ms = now_ms();
            ipc_send_event(ipc, &ev);
            continue;
        }

        if (m.type == MSG_REPARE) {
            out_of_service = 0;
            printf("[ASC %d] Réparé. Reprise du service.\n", asc_id);

            MessageIPC ev = {0};
            ev.type = MSG_EVENT;
            ev.asc_id = asc_id;
            ev.data.event.evt = EVT_REPARE;
            ev.data.event.etage = -1;
            ev.data.event.t_ms = now_ms();
            ipc_send_event(ipc, &ev);
            continue;
        }

        if (m.type == MSG_MISSION) {
            Demande d = m.data.mission;

            if (out_of_service) {
                printf("[ASC %d] Refus mission (HS) demande #%d %d->%d\n",
                       asc_id, d.id, d.from, d.to);

                MessageIPC ev = {0};
                ev.type = MSG_EVENT;
                ev.asc_id = asc_id;
                ev.data.event.evt = EVT_REFUS;
                ev.data.event.etage = -1;
                ev.data.event.demande = d;
                ev.data.event.t_ms = now_ms();
                ipc_send_event(ipc, &ev);
                continue;
            }

            printf("[ASC %d] mission: %d -> %d\n", asc_id, d.from, d.to);

            sleep(1);

            MessageIPC ev = {0};
            ev.type = MSG_EVENT;
            ev.asc_id = asc_id;
            ev.data.event.evt = EVT_DROPOFF;
            ev.data.event.etage = d.to;
            ev.data.event.demande = d;     // ✅ essentiel pour stats
            ev.data.event.t_ms = now_ms();

            ipc_send_event(ipc, &ev);
        }
    }

    printf("[ASC %d] terminé.\n", asc_id);
}
