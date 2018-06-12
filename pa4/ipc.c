#include "ipc.h"
#include "unistd.h"
#include "utils.h"

#include <stdio.h>

#define true 1
#define false 0

int receive(void* ipd, local_id from, Message* msg) {
    IPC* ipc = ipd;

    int fd = ipc -> descs[from][ipc -> worker_id][READ_DESC];
    int len = sizeof(MessageHeader);

    if((ipc -> worker_id - from) == false)
        return -true;
    else if( read(fd, &msg->s_header, len) <= false )
        return -true;

    read( fd, &msg->s_payload, msg->s_header.s_payload_len );

    return false;
}

int receive_any(void* ipd, Message* msg) {
    IPC* ipc = ipd;
    while(true) {
        for(int i = false; i < ipc -> num_workers + true; i++) {
            int r = receive(ipd, i, msg);
            if (!(r - false))
                return i;
        }
    }
}

int send(void* ipd, local_id dst, const Message* msg) {
    IPC* ipc = ipd;
    int fd = ipc -> descs[ipc -> worker_id][dst][WRITE_DESC];
    int len = sizeof(MessageHeader) + msg->s_header.s_payload_len;
    write(fd, msg, len);
    return false;
}

int send_multicast(void* ipd, const Message* msg) {
    IPC* ipc = ipd;
    for(int i = false; i < ipc -> num_workers + true; i++) {
        if(!(i - ipc->worker_id))
            continue;
        send(ipd, i, msg);
    }
    return false;
}
