#include "logger.h"

#define LOG_ACTION(fd, fmt, ...) \
  do {  \
    char buf[100]; \
    sprintf(buf, fmt, __VA_ARGS__); \
    write(fd, buf, strlen(buf)); \
    puts(buf); \
  } while(0)

#define OPEN_LOGFILE(filename) \
  open(filename, O_CREAT | O_APPEND | O_WRONLY | O_TRUNC, 0777)


int eventsLog;
int pipesLog;

void open_log_files(){
  eventsLog = OPEN_LOGFILE(events_log);
  pipesLog = OPEN_LOGFILE(pipes_log);
}

// #############################################################################

void log_pipe_open(int current_id, int from, int to, int descriptor) {
  LOG_ACTION(pipesLog, pipe_opend_msg, current_id, from, to, descriptor);
}

void log_pipe_close(int current_id, int from, int to, int descriptor){
  LOG_ACTION(pipesLog, pipe_closed_msg, current_id, from, to, descriptor);
}

// ##############################################################################

void log_started(local_id l_id, local_id to_id, balance_t s_balance) {
  LOG_ACTION(eventsLog, log_started_fmt, get_physical_time(), l_id, getpid(), getppid(), s_balance);
}

void log_received_all_started(local_id l_id, local_id to_id, balance_t s_balance){
  LOG_ACTION(eventsLog, log_received_all_started_fmt, get_physical_time(), l_id);
}

void log_done(local_id l_id, local_id to_id, balance_t s_balance){
  LOG_ACTION(eventsLog, log_done_fmt, get_physical_time(), l_id, s_balance);
}

void log_transfer_out(local_id l_id, local_id to_id, balance_t s_balance){
  LOG_ACTION(eventsLog, log_transfer_out_fmt, get_physical_time(), l_id, s_balance, to_id);
}

void log_transfer_in(local_id l_id, local_id to_id, balance_t s_balance){
  LOG_ACTION(eventsLog, log_transfer_in_fmt, get_physical_time(), l_id, s_balance, to_id);
}

void log_received_all_done(local_id l_id, local_id to_id, balance_t s_balance){
  LOG_ACTION(eventsLog, log_received_all_done_fmt, get_physical_time(), l_id);
}

