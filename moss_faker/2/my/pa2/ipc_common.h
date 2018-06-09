#ifndef IPC_COMMON_HEADER_H
#define IPC_COMMON_HEADER_H

#include <stdint.h>
#include <stddef.h>
#include "banking.h"

typedef struct{
	int s_read_fd;
	int s_write_fd;
} PipeFd;

typedef struct{
	balance_t s_balance;
	local_id s_current_id;
	PipeFd *s_pipes[10][10];
	int s_process_count;
} InteractionInfo;

int receive_multicast(void * self, int16_t type);

#endif
