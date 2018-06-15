#include "ipc.h"

timestamp_t lamport_time;

timestamp_t get_lamport_time();

void init_lamport_time();

void inc_lamport_time();

void set_lamport_time(timestamp_t local_time);
