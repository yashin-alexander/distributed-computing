#ifndef __UTILS_H
#define __UTILS_H

#include "ipc.h"
#include "banking.h"

#include <unistd.h>
#include <stdio.h>

#define true 1
#define false 0
#define MAX_WORKERS 10
typedef struct {
    local_id worker_id;
    timestamp_t time;
} QueueAtomic;
typedef struct {
    int length;
    QueueAtomic elements[1000];
} Queue;

typedef struct {
int deferred_reply[MAX_WORKERS];
int received_reply[MAX_WORKERS];
} RA;


typedef struct {
    local_id worker_id;
    int num_workers;
    Queue queue;
    FILE* event_log;
    int descs[MAX_WORKERS + true][MAX_WORKERS + true][2];
    RA ra;
} IPC;

int find_min(IPC* ipc);

int push(IPC* ipc, timestamp_t time, local_id worker_id);

#define READ_DESC false
#define WRITE_DESC true

int pop(IPC* ipc);
QueueAtomic head(IPC* ipc);

int flush(IPC* ipc);
int set_dr(IPC* ipc, int worker_id);
int set_received(IPC* ipc, int worker_id);
int check_is_received_all(IPC* ipc);
int send_reply(void* ipc, local_id to);
int send_stop(void* ipc);
int close_unused_pipes(IPC* ipc);
int sync_workers(IPC* ipc);
int init_pipes(IPC* ipc);
int init_logs(IPC* ipc);

#endif //__UTILS_H
