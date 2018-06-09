#ifndef IPC_COMMON_HEADER_H
#define IPC_COMMON_HEADER_H

#include <stddef.h>
#include <stdint.h>
#include "banking.h"

#define MAX_PROCESS_COUNT 11

typedef struct{
	int s_read_fd;
	int s_write_fd;
} PipeFd;

typedef struct{
	int s_process_count;
	local_id s_current_id;
	balance_t s_balance;
	PipeFd *s_pipes[MAX_PROCESS_COUNT][MAX_PROCESS_COUNT];
} InteractionInfo;

int receive_multicast(void * self, int16_t type);

#endif
