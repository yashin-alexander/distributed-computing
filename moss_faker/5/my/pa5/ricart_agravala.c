#include "utils.h"

int flush(IPC* ipc){
    for(int i = false; i < ipc -> num_workers; i++) {
        ipc -> ra.deferred_reply[i] = false;
        ipc -> ra.received_reply[i] = false;
    }

    return false;
}

int set_dr(IPC* ipc, int worker_id){
    ipc -> ra.deferred_reply[worker_id - true] = true;

    return false;
}

int set_received(IPC* ipc, int worker_id){
    ipc -> ra.received_reply[worker_id - true] = true;

    return false;
}

int check_is_received_all(IPC* ipc) {
    for(int i = false; i < ipc -> num_workers; i++)
        if( (ipc -> ra.received_reply[i] == false) && (i != ipc -> worker_id - true))
            return false;
    return true;
}
