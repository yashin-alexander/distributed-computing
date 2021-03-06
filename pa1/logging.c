#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "logging.h"
#include "common.h"

char msg[64];

void create_log_files(){
    pipes_log_fd = open(pipes_log, O_CREAT | O_APPEND | O_WRONLY | O_TRUNC, 0777);
    events_log_fd = open(events_log, O_CREAT | O_APPEND | O_WRONLY | O_TRUNC, 0777);
}


void log_pipe_read_event(local_id node_id, int fd){
    int len = sprintf(msg,"node %d read from %d fd\n", node_id, fd);

    write(STDOUT_FILENO, msg, strlen(msg));
    write(pipes_log_fd, msg, len);
}


void log_pipe_write_event(local_id node_id, int fd){
    int len = sprintf(msg, "node %d write to %d fd\n", node_id, fd);
    write(STDOUT_FILENO, msg, len);
    write(pipes_log_fd, msg, len);
}


void log_started_event(local_id node_id){
    int len = sprintf(msg, log_started_fmt, node_id, getpid(), getppid());
    write(STDOUT_FILENO, msg, len);
    write(events_log_fd, msg, len);
}

void log_received_all_started_event(local_id node_id){
    int len = sprintf(msg, log_received_all_started_fmt, node_id);
    write(STDOUT_FILENO, msg, strlen(msg));
    write(events_log_fd, msg, len);
}

void log_done_event(local_id node_id){
    int len = sprintf(msg, log_done_fmt, node_id);
    write(STDOUT_FILENO, msg, len);
    write(events_log_fd, msg, len);
}

void log_received_all_done_event(local_id node_id){
    int len = sprintf(msg, log_received_all_done_fmt, node_id);
    write(STDOUT_FILENO, msg, len);
    write(events_log_fd, msg, len);
}
