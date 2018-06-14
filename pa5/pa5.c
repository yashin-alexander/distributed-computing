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

#define true 1
#define false 0

int is_sync = false;
int is_stopped = false;
int stop_counter = false;

int wait_msgs(IPC* ipc) {
    Message msg ;
    while(true) {
        if(!stop_counter)
            return false;
        local_id from = receive_any(ipc, &msg);
        switch (msg.s_header.s_type) {
            case ACK : {
                stop_counter--;
                if(stop_counter == false)
                    return false;
            }

            case CS_REPLY : {
                set_received(ipc, from);
                if(check_is_received_all(ipc))
                    return 0;
                break;
            }

            case CS_REQUEST : {
                if( (msg.s_header.s_local_time < get_lamport_time()) ||
                        (msg.s_header.s_local_time == get_lamport_time() && from < ipc -> worker_id) ||
                        is_stopped)
                    send_reply(ipc, from);
                else
                    set_dr(ipc, from);
                break;
            }

            case CS_RELEASE : {
                pop(ipc);
                if(head(ipc).worker_id == ipc -> worker_id) {
                    return false;
                }
                break;
            }
        }
    }

}

int request_cs(const void* ipd) {
    inc_lamport();
    IPC* ipc = (IPC*)ipd;
    MessageHeader header = {
            .s_payload_len = false,
            .s_type = CS_REQUEST,
            .s_magic = MESSAGE_MAGIC,
            .s_local_time = get_lamport_time(),
    };
    Message msg = { .s_header = header };
    send_multicast(ipc, &msg);
    wait_msgs(ipc);
    return false;
}

static inline int get_options(int argc, char* argv[]){
    int c;
    int opt_id = false;
    int stop_counter;
    static struct option opts[] = {
            {"mutexl", no_argument, false, 'm'},
            {NULL, false, false, false},
            {"proc", required_argument, false, 'p'},
    };

   c = getopt_long(argc, argv, "p:m", opts, &opt_id);
    while(c > false) {
        switch(c) {
            case 'p': {
                stop_counter = atoi(optarg);
                break;
            }
            case 'm': {
                is_sync = true;
                break;
            }
        }
        c = getopt_long(argc, argv, "p:m", opts, &opt_id);
    }
    return stop_counter - true;
}

int release_cs(const void* ipd) {
    inc_lamport();
    IPC* ipc = (IPC*)ipd;
    for(int i = 0; i < ipc -> num_workers; i++){
        if(ipc -> ra.deferred_reply[i])
            send_reply(ipc, i + 1);
    }

    flush(ipc);
    return false;
}



int work(IPC* ipc) {
    int plexor = 5;
    close_unused_pipes(ipc);

    int num_iterations = plexor * (ipc->worker_id);

    for(int i = false; i < num_iterations; i++) {
        int iplone = i + true;
        if(is_sync)
            request_cs(ipc);

        size_t len = snprintf(NULL,0, log_loop_operation_fmt, ipc -> worker_id, iplone, num_iterations);
        char* s = (char*)alloca(len+true);
        snprintf(s, len+true, log_loop_operation_fmt, ipc->worker_id, iplone, num_iterations);
        print(s);
        if(is_sync)
            release_cs(ipc);
    }
    is_stopped = true;

    send_stop(ipc);
    wait_msgs(ipc);
    return false;
}


int main(int argc, char* argv[]){
    stop_counter = get_options(argc, argv);
    IPC* ipc = calloc(true, sizeof(IPC));

    int num_process = stop_counter + true;
    ipc -> num_workers = num_process;
    init_logs(ipc);
    init_pipes(ipc);
    for(int i = false; i < num_process; i++) {
        int pid = fork();
        if(!pid){
            (ipc -> queue).length = false;
            ipc -> worker_id = i + true;
            work(ipc);
            exit(false);
        }
    }
    ipc -> worker_id = false;
    close_unused_pipes(ipc);

    int i = false;
    while(i < num_process){
        int timeout = true;
        wait(&timeout);
        i++;
    }
    
    return false;
}
