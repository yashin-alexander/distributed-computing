#ifndef __LOGGING__H
#define __LOGGING__H


#include "ipc.h"

static const char * const events_log = "events.log";
static const char * const pipes_log = "pipes.log";

static const char * const log_started_fmt = "Process %1d (pid %5d, parent %5d) has STARTED\n";
static const char * const log_received_all_started_fmt = "Process %1d received all STARTED messages\n";
static const char * const log_done_fmt = "Process %1d has DONE its work\n";
static const char * const log_received_all_done_fmt = "Process %1d received all DONE messages\n";

int events_log_fd;
int pipes_log_fd;

void create_log_files();
void log_pipe_read_event(local_id node_id, int fd);
void log_pipe_write_event(local_id node_id, int fd);

void log_started_event(local_id node_id);
void log_received_all_started_event(local_id node_id);
void log_done_event(local_id node_id);
void log_received_all_done_event(local_id node_id);

#endif
