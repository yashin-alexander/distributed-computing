#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "pa1.h"
#include "common.h"

#include "logging.h"

int get_pipes_log_descriptor() {
    static int pipes_log_descriptor = -1;
    if (pipes_log_descriptor < 0) {
        pipes_log_descriptor = open(pipes_log, O_CREAT | O_APPEND | O_WRONLY | O_TRUNC, 0777);
    }
    return pipes_log_descriptor;
}

void log_pipe_event(char *fmt, local_id node_id, int fd) {
    char formated_message[64];
    int len = sprintf(formated_message, fmt, node_id, fd);

    puts(formated_message);
    write(get_pipes_log_descriptor(), formated_message, len);
}

void log_pipe_read_event(local_id node_id, int fd) {
    log_pipe_event("Node %d is reading from %d file descriptor\n", node_id, fd);
}

void log_pipe_write_event(local_id node_id, int fd) {
    log_pipe_event("Node %d is writing to %d file descriptor\n", node_id, fd);
}

int get_events_log_descriptor() {
    static int events_log_descriptor = -1;
    if (events_log_descriptor < 0) {
        events_log_descriptor = open(events_log, O_CREAT | O_APPEND | O_WRONLY | O_TRUNC, 0777);
    }
    return events_log_descriptor;
}

void log_event(char *formated_message, int message_len) {
    puts(formated_message);
    write(get_events_log_descriptor(), formated_message, message_len);
}

void log_started_event(local_id node_id) {
    char buffer[64];
    int len = sprintf(buffer, log_started_fmt, node_id, getpid(), getppid());
    log_event(buffer, len);
}

void log_received_all_started_event(local_id node_id) {
    char buffer[64];
    int len = sprintf(buffer, log_received_all_started_fmt, node_id);
    log_event(buffer, len);
}

void log_done_event(local_id node_id) {
    char buffer[64];
    int len = sprintf(buffer, log_done_fmt, node_id);
    log_event(buffer, len);
}

void log_received_all_done_event(local_id node_id) {
    char buffer[64];
    int len = sprintf(buffer, log_received_all_done_fmt, node_id);
    log_event(buffer, len);
}
