#include "lamport.h"

timestamp_t get_lamport_time(){
    return lamport_time;
}

void init_lamport_time(){
    lamport_time = 0;
}

void inc_lamport_time(){
    lamport_time++;
}

void set_lamport_time(timestamp_t local_time){
    lamport_time = (local_time>lamport_time?local_time+1: lamport_time+1);
}
