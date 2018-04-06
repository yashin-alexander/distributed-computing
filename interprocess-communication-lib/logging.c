#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "pa1.h"
#include "common.h"

#include "logging.h"

void create_log_files(){
    pipes_log_fd = open(pipes_log, O_CREAT | O_APPEND | O_WRONLY | O_TRUNC, 0777);
    events_log_fd = open(events_log, O_CREAT | O_APPEND | O_WRONLY | O_TRUNC, 0777);
}


void log_pipe_read_event(local_id node_id, int fd){
    char msg[64];
    int len = sprintf(msg,"node %d read from %d fd\n", node_id, fd);

    write(0, msg, strlen(msg));
    write(pipes_log_fd, msg, len);
}


void log_pipe_write_event(local_id node_id, int fd){
    char msg[64];
    int len = sprintf(msg, "node %d write to %d fd\n", node_id, fd);
    write(0, msg, len);
    write(pipes_log_fd, msg, len);
}


void log_started_event(local_id node_id){
    char msg[64];
    int len = sprintf(msg, log_started_fmt, node_id, getpid(), getppid());
    write(0, msg, len);
    write(events_log_fd, msg, len);
}

void log_received_all_started_event(local_id node_id){
    char msg[64];
    int len = sprintf(msg, log_received_all_started_fmt, node_id);
    write(0, msg, strlen(msg));
    write(events_log_fd, msg, len);
}

void log_done_event(local_id node_id){
    char msg[64];
    int len = sprintf(msg, log_done_fmt, node_id);
    write(0, msg, len);
    write(events_log_fd, msg, len);
}

void log_received_all_done_event(local_id node_id){
    char msg[64];
    int len = sprintf(msg, log_received_all_done_fmt, node_id);
    write(0, msg, len);
    write(events_log_fd, msg, len);
}
