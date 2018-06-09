#include "logger.h"


int pipesLog;
int eventsLog;

void open_log_files(){
  eventsLog = open(events_log, O_CREAT | O_APPEND | O_WRONLY | O_TRUNC, 0777);
  pipesLog = open(pipes_log, O_CREAT | O_APPEND | O_WRONLY | O_TRUNC, 0777);
}

// #############################################################################

void log_pipe_open(int current_id, int from, int to, int descriptor) {
  char buf[100];
  sprintf(buf, pipe_opend_msg, current_id, from, to, descriptor);
  write(pipesLog, buf, strlen(buf));
}

void log_pipe_close(int current_id, int from, int to, int descriptor){
  char buf[100];
  sprintf(buf, pipe_closed_msg, current_id, from, to, descriptor);
  write(pipesLog, buf, strlen(buf));
}

// ##############################################################################

void log_started(local_id l_id, local_id to_id, balance_t s_balance) {
  char buf[MAX_MESSAGE_LEN];
  sprintf(buf, log_started_fmt, get_physical_time(), l_id, getpid(), getppid(), s_balance);
  printf("%s", buf);
  write(eventsLog, buf, strlen(buf));
}

void log_received_all_started(local_id l_id, local_id to_id, balance_t s_balance){
  char buf[MAX_MESSAGE_LEN];
  sprintf(buf, log_received_all_started_fmt, get_physical_time(), l_id);
  printf("%s", buf);
  write(eventsLog, buf, strlen(buf));
}

void log_done(local_id l_id, local_id to_id, balance_t s_balance){
  char buf[MAX_MESSAGE_LEN];
  sprintf(buf, log_done_fmt, get_physical_time(), l_id, s_balance);
  printf("%s", buf);
  write(eventsLog, buf, strlen(buf));
}

void log_transfer_out(local_id l_id, local_id to_id, balance_t s_balance){
  char buf[MAX_MESSAGE_LEN];
  sprintf(buf, log_transfer_out_fmt, get_physical_time(), l_id, s_balance, to_id);
  printf("%s", buf);
  write(eventsLog, buf, strlen(buf));
}

void log_transfer_in(local_id l_id, local_id to_id, balance_t s_balance){
  char buf[MAX_MESSAGE_LEN];
  sprintf(buf, log_transfer_in_fmt, get_physical_time(), l_id, s_balance, to_id);
  printf("%s", buf);
  write(eventsLog, buf, strlen(buf));
}

void log_received_all_done(local_id l_id, local_id to_id, balance_t s_balance){
  char buf[MAX_MESSAGE_LEN];
  sprintf(buf, log_received_all_done_fmt, get_physical_time(), l_id);
  printf("%s", buf);
  write(eventsLog, buf, strlen(buf));
}

