#include "ipc.h"

#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <stdio.h>

static int create_or_get_queue(const char *path, int proj_id) {
    key_t key = ftok(path, proj_id);
    if (key == (key_t)-1) {
        perror("ftok");
        return -1;
    }
    int qid = msgget(key, IPC_CREAT | 0666);
    if (qid == -1) {
        perror("msgget");
        return -1;
    }
    return qid;
}

int ipc_init(IPC *ipc, const char *path_for_ftok, int proj_ctrl2asc, int proj_asc2ctrl) {
    if (!ipc || !path_for_ftok) return -1;

    ipc->q_ctrl_to_asc = create_or_get_queue(path_for_ftok, proj_ctrl2asc);
    if (ipc->q_ctrl_to_asc < 0) return -1;

    ipc->q_asc_to_ctrl = create_or_get_queue(path_for_ftok, proj_asc2ctrl);
    if (ipc->q_asc_to_ctrl < 0) return -1;

    return 0;
}

void ipc_cleanup(IPC *ipc) {
    if (!ipc) return;

    if (ipc->q_ctrl_to_asc >= 0) {
        if (msgctl(ipc->q_ctrl_to_asc, IPC_RMID, NULL) == -1) {
            perror("msgctl IPC_RMID ctrl->asc");
        }
    }
    if (ipc->q_asc_to_ctrl >= 0) {
        if (msgctl(ipc->q_asc_to_ctrl, IPC_RMID, NULL) == -1) {
            perror("msgctl IPC_RMID asc->ctrl");
        }
    }
}

static int send_raw(int qid, long mtype, const MessageIPC *m) {
    MsgSysV packet;
    packet.mtype = mtype;
    packet.msg = *m;

    if (msgsnd(qid, &packet, sizeof(MessageIPC), 0) == -1) {
        perror("msgsnd");
        return -1;
    }
    return 0;
}

static int recv_raw(int qid, long mtype, MessageIPC *out, int block) {
    MsgSysV packet;
    int flags = block ? 0 : IPC_NOWAIT;

    ssize_t r = msgrcv(qid, &packet, sizeof(MessageIPC), mtype, flags);
    if (r == -1) {
        if (!block && errno == ENOMSG) return 1; // rien à lire
        perror("msgrcv");
        return -1;
    }
    *out = packet.msg;
    return 0;
}

int ipc_send_mission(const IPC *ipc, int asc_id, const MessageIPC *m) {
    if (!ipc || !m) return -1;
    return send_raw(ipc->q_ctrl_to_asc, MTYPE_ASC(asc_id), m);
}

int ipc_recv_mission(const IPC *ipc, int asc_id, MessageIPC *out, int block) {
    if (!ipc || !out) return -1;
    return recv_raw(ipc->q_ctrl_to_asc, MTYPE_ASC(asc_id), out, block);
}

int ipc_send_event(const IPC *ipc, const MessageIPC *m) {
    if (!ipc || !m) return -1;
    return send_raw(ipc->q_asc_to_ctrl, MTYPE_CTRL, m);
}

int ipc_recv_event(const IPC *ipc, MessageIPC *out, int block) {
    if (!ipc || !out) return -1;
    return recv_raw(ipc->q_asc_to_ctrl, MTYPE_CTRL, out, block);
}
