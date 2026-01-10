#include "ascenseur.h"
#include "common.h"
#include "ipc.h"
#include "statistiques.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

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

static void send_event_simple(const IPC *ipc, int asc_id, TypeEvent evt, int etage) {
    MessageIPC ev = {0};
    ev.type = MSG_EVENT;
    ev.asc_id = asc_id;
    ev.data.event.evt = evt;
    ev.data.event.etage = etage;
    ev.data.event.t_ms = now_ms();
    ipc_send_event(ipc, &ev);
}

static void send_refus(const IPC *ipc, int asc_id, const Demande *d, const char *why) {
    MessageIPC ev = {0};
    ev.type = MSG_EVENT;
    ev.asc_id = asc_id;
    ev.data.event.evt = EVT_REFUS;
    ev.data.event.etage = -1;
    ev.data.event.demande = *d;
    ev.data.event.t_ms = now_ms();
    ipc_send_event(ipc, &ev);

    char buf[LOG_LEN];
    snprintf(buf, sizeof(buf), "[ASC %d] Refus %s demande #%d %d->%d",
             asc_id, why, d->id, d->from, d->to);
    send_log(ipc, asc_id, buf);
}

static void drain_pending_missions_as_refus(const IPC *ipc, int asc_id) {
    while (1) {
        MessageIPC m2;
        int r2 = ipc_recv_mission(ipc, asc_id, &m2, 0);
        if (r2 == 1) break;
        if (r2 != 0) break;

        if (m2.type == MSG_MISSION) {
            Demande d = m2.data.mission;
            send_refus(ipc, asc_id, &d, "(drain HS)");
        }
    }
}

void ascenseur_run(int asc_id, const IPC *ipc) {
    // log démarrage (centralisé)
    char buf[LOG_LEN];
    snprintf(buf, sizeof(buf), "[ASC %d] démarré (pid=%d)", asc_id, getpid());
    send_log(ipc, asc_id, buf);

    // READY
    send_event_simple(ipc, asc_id, EVT_READY, -1);

    int out_of_service = 0;

    while (1) {
        MessageIPC m;
        int r = ipc_recv_mission(ipc, asc_id, &m, 1);
        if (r != 0) continue;

        if (m.type == MSG_STOP) {
            snprintf(buf, sizeof(buf), "[ASC %d] STOP reçu, arrêt.", asc_id);
            send_log(ipc, asc_id, buf);
            break;
        }

        if (m.type == MSG_PANNE) {
            out_of_service = 1;

            snprintf(buf, sizeof(buf), "[ASC %d] PANNE ! Hors service.", asc_id);
            send_log(ipc, asc_id, buf);

            send_event_simple(ipc, asc_id, EVT_PANNE, -1);

            // refuse tout ce qui est en attente
            drain_pending_missions_as_refus(ipc, asc_id);
            continue;
        }

        if (m.type == MSG_REPARE) {
            out_of_service = 0;

            snprintf(buf, sizeof(buf), "[ASC %d] Réparé. Reprise du service.", asc_id);
            send_log(ipc, asc_id, buf);

            send_event_simple(ipc, asc_id, EVT_REPARE, -1);
            continue;
        }

        if (m.type == MSG_MISSION) {
            Demande d = m.data.mission;

            if (out_of_service) {
                send_refus(ipc, asc_id, &d, "(HS)");
                continue;
            }

            snprintf(buf, sizeof(buf), "[ASC %d] mission: %d -> %d", asc_id, d.from, d.to);
            send_log(ipc, asc_id, buf);

            sleep(1);

            MessageIPC ev = {0};
            ev.type = MSG_EVENT;
            ev.asc_id = asc_id;
            ev.data.event.evt = EVT_DROPOFF;
            ev.data.event.etage = d.to;
            ev.data.event.demande = d;
            ev.data.event.t_ms = now_ms();
            ipc_send_event(ipc, &ev);
        }
    }

    snprintf(buf, sizeof(buf), "[ASC %d] terminé.", asc_id);
    send_log(ipc, asc_id, buf);
}
