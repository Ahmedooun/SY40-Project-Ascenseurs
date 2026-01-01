#ifndef IPC_H
#define IPC_H

#include <sys/types.h>
#include "common.h"

typedef struct {
    int q_ctrl_to_asc;
    int q_asc_to_ctrl;
} IPC;

#define MTYPE_ASC(asc_id) (1L + (long)(asc_id))
#define MTYPE_CTRL        (1L)

typedef struct {
    long mtype;
    MessageIPC msg;
} MsgSysV;

int  ipc_init(IPC *ipc, const char *path_for_ftok, int proj_ctrl2asc, int proj_asc2ctrl);
void ipc_cleanup(IPC *ipc);

int ipc_send_mission(const IPC *ipc, int asc_id, const MessageIPC *m);
int ipc_recv_mission(const IPC *ipc, int asc_id, MessageIPC *out, int block);

int ipc_send_event(const IPC *ipc, const MessageIPC *m);
int ipc_recv_event(const IPC *ipc, MessageIPC *out, int block);

#endif
