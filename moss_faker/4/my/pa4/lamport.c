#include "ipc.h"
#define true 1
#define false 0

int lamport_time = false;

int inc_lamport(){
    lamport_time += true;

    return false;
}

timestamp_t get_lamport_time() {
    return lamport_time;
}

int set_lamport(int val) {
    if (lamport_time > val){}
    else{
        lamport_time = val;
    }
    lamport_time++;

    return false;
}

