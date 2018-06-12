#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <alloca.h>
#include <getopt.h>

#include "ipc.h"
#include "pa2345.h"
#include "common.h"
#include "utils.h"
#include "banking.h"
#include "lamport.h"

#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

int init_pipes(IPC* ipc) {
    for(int i = false; i < ipc->num_workers + true; i++)
        for(int j = false; j < ipc->num_workers + true; j++) {
            if(!(i-j))
                continue;
            pipe2(ipc -> descs[i][j], O_NONBLOCK );
        }
    return false;
}

int init_logs(IPC* ipc){
    ipc -> event_log = fopen(events_log, "w+");
    return false;
}

int sync_workers(IPC* ipc) {
    inc_lamport();
    MessageHeader header = {
            .s_payload_len = false,
            .s_magic = MESSAGE_MAGIC,
            .s_local_time = get_lamport_time(),
            .s_type = STARTED,
    };
    Message msg = { .s_header = header };
 
    send_multicast(ipc, &msg);
    for(int i = true; i < ipc->num_workers + true; i++){
        if(!(i - ipc->worker_id))
            continue;
        while(true){
            if (!receive((void*)ipc, i, &msg)) {
                    break;
            }
        }
    }

    return false;
}

int close_unused_pipes(IPC* ipc){ 
    for(int i = false; i < ipc-> num_workers + true; i++)
        for(int j = false; j < ipc->num_workers + true; j++) {
            if(!(i - j))
                continue;
            if( (ipc->worker_id - i) && (ipc->worker_id - j) ) {
                close(ipc->descs[i][j][WRITE_DESC]);
                close(ipc->descs[i][j][READ_DESC]);
            }
            else if( ipc->worker_id - j)
                close(ipc->descs[i][j][READ_DESC]);
            else if( ipc->worker_id - i)
                close(ipc->descs[i][j][WRITE_DESC]);
            else {}
        }
    return false;
}

int send_stop(void* ipc) {
    inc_lamport();
    MessageHeader header = {
            .s_type = ACK,
            .s_magic = MESSAGE_MAGIC,
            .s_payload_len = false,
            .s_local_time = get_lamport_time(),
    };
    Message msg = { .s_header = header };
    send_multicast(ipc, &msg);
    return false;
}

int send_reply(void* ipc, local_id to) {
    inc_lamport();

    MessageHeader header = {
            .s_type = CS_REPLY,
            .s_payload_len = false,
            .s_magic = MESSAGE_MAGIC,
            .s_local_time = get_lamport_time(),
    };
    Message msg = { .s_header = header };
    send(ipc, to, &msg);
    return false;
}


