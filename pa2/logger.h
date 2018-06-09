#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "banking.h"
#include "common.h"
#include "pa2345.h"
#include "ipc.h"

static const char * const pipe_opend_msg = "Pipe for %d from %d to %d and descriptor %d opend\n";
static const char * const pipe_closed_msg = "Pipe for %d from %d to %d and descriptor %d closed\n";

void open_log_files();

void log_pipe_open(int current_id, int from, int to, int descriptor);
void log_pipe_close(int current_id, int from, int to, int descriptor);


void log_started(local_id l_id, local_id to_id, balance_t s_balance);
void log_received_all_started(local_id l_id, local_id to_id, balance_t s_balance);
void log_done(local_id l_id, local_id to_id, balance_t s_balance);
void log_transfer_out(local_id l_id, local_id to_id, balance_t s_balance);
void log_transfer_in(local_id l_id, local_id to_id, balance_t s_balance);
void log_received_all_done(local_id l_id, local_id to_id, balance_t s_balance);
