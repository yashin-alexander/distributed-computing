#ifndef __LOGGING__H
#define __LOGGING__H


#include "ipc.h"

int events_log_fd;
int pipes_log_fd;

int get_pipes_log_descriptor();
void log_pipe_event(char *fmt, local_id node_id, int fd);
void log_pipe_read_event(local_id node_id, int fd);
void log_pipe_write_event(local_id node_id, int fd);

int get_events_log_descriptor();
void log_event(char *formated_message, int message_len);
void log_started_event(local_id node_id);
void log_received_all_started_event(local_id node_id);
void log_done_event(local_id node_id);
void log_received_all_done_event(local_id node_id);

#endif
