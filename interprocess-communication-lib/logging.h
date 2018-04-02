#ifndef __LOGGING__H
#define __LOGGING__H


int events_log_fd;
int pipes_log_fd;

void create_log_files();

void logging_start(int logging_fd, int id);
void logging_received_all_started(int logging_fd, int id);
void logging_received_all_done(int logging_fd, int id);
void logging_done(int logging_fd, int id);

void logging_pipes_open(int logging_fd, int from_index, int to_index, int pipe_in, int pipe_out);
void logging_pipe_read(int logging_fd, int node_index, int pipe);
void logging_pipe_write(int logging_fd, int node_index, int pipe);

#endif
