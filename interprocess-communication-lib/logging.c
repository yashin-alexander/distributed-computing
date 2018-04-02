#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "pa1.h"
#include "common.h"

#include "logging.h"


void create_log_files(){
    pipes_log_fd = open(pipes_log, O_CREAT | O_APPEND | O_WRONLY | O_TRUNC, 0777);
    events_log_fd = open(events_log, O_CREAT | O_APPEND | O_WRONLY | O_TRUNC, 0777);
}

void logging_start(int logging_fd, int id){
    char msg[64];
    int msg_len;

    msg_len = sprintf(msg, log_started_fmt, id, getpid(), getppid());
    write(logging_fd, msg, msg_len);
    write(STDOUT_FILENO, msg, msg_len);
}

void logging_received_all_started(int logging_fd, int id){
    char msg[64];
    int msg_len;

    msg_len = sprintf(msg, log_received_all_started_fmt, id);
    write(logging_fd, msg, msg_len);
    write(STDOUT_FILENO, msg, msg_len);
}


void logging_received_all_done(int logging_fd, int id){
    char msg[64];
    int msg_len;

    msg_len = sprintf(msg, log_received_all_done_fmt, id);
    write(logging_fd, msg, msg_len);
    write(STDOUT_FILENO, msg, msg_len);
}


void logging_done(int logging_fd, int id){
    char msg[64];
    int msg_len;

    msg_len = sprintf(msg, log_done_fmt, id);
    write(logging_fd, msg, msg_len);
    write(STDOUT_FILENO, msg, msg_len);
}

void logging_pipes_open(int logging_fd, int from_index, int to_index, int pipe_in, int pipe_out){
    char msg[64];
    int msg_len;

    msg_len = sprintf(msg, "Pipe from %d to %d created. in=%d out=%d\n",
                      from_index, to_index, pipe_in, pipe_out);
    write(logging_fd, msg, msg_len);
}

void logging_pipe_read(int logging_fd, int node_index, int pipe){
    char msg[64];
    int msg_len;

    msg_len = sprintf(msg, "Node %d read from %d pipe\n", node_index, pipe);
    write(logging_fd, msg, msg_len);
}

void logging_pipe_write(int logging_fd, int node_index, int pipe){
    char msg[64];
    int msg_len;

    msg_len = sprintf(msg, "Node %d write to %d pipe\n", node_index, pipe);
    write(logging_fd, msg, msg_len);
}
