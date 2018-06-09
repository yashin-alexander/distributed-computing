#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>

#include "ipc.h"
#include "common.h"
#include "pa2345.h"
#include "ipc_common.h"


void init_array(InteractionInfo* interaction_info);

void open_pipes(InteractionInfo* interaction_info);

pid_t* fork_processes(int process_count, InteractionInfo* interaction_info, int *balances);

void close_redundant_pipes(InteractionInfo* interaction_info);

void close_self_pipes(InteractionInfo* interaction_info);

MessageHeader create_message_header(uint16_t magic, uint16_t len, int16_t type, timestamp_t time);

Message create_message(uint16_t magic, char* payload, uint16_t len, int16_t type, timestamp_t time);
