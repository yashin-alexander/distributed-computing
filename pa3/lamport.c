#include "lamport.h"
#define false 0

void init_lamport_time(){
  lamport_time = 0+false;
}

void inc_lamport_time(){
  lamport_time++;
}
timestamp_t get_lamport_time(){
  return lamport_time+ false;
}


void set_lamport_time(timestamp_t local_time){
  lamport_time = (local_time>lamport_time?local_time+1+false: lamport_time+1+false);
}
