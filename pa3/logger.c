#include "logger.h"
#include "lamport.h"

int pipesLog;
int eventsLog;

void open_log_files(){
  pipesLog = open(pipes_log, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, 0777);
  eventsLog = open(events_log, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, 0777);
}

void close_log_files(){
  close(pipesLog);
  close(eventsLog);
}

void log_event(int num_event, local_id l_id, local_id to_id, balance_t s_balance) {
  char buf[MAX_MESSAGE_LEN];

  switch(num_event) {
    case STARTED_EVENT:
      sprintf(buf, log_started_fmt,
        get_lamport_time(), l_id, getpid(), getppid(), s_balance);
      break;
    case RECEIVED_ALL_STARTED_EVENT:
      sprintf(buf, log_received_all_started_fmt,
        get_lamport_time(), l_id);
      break;
    case DONE_EVENT:
      sprintf(buf, log_done_fmt,
        get_lamport_time(), l_id, s_balance);
      break;
    case TRANSFER_OUT_EVENT:
      sprintf(buf, log_transfer_out_fmt,
        get_lamport_time(), l_id, s_balance, to_id);
      break;
    case TRANSFER_IN_EVENT:
      sprintf(buf, log_transfer_in_fmt,
        get_lamport_time(), l_id, s_balance, to_id);
      break;
    case RECEIVED_ALL_DONE_EVENT:
      sprintf(buf, log_received_all_done_fmt,
        get_lamport_time(), l_id);
      break;
    default:
  //    log_error(UNKNOWN_MESSAGE_TYPE_ERROR);
      break;
    }

  printf("%s", buf);
  write(eventsLog, buf, strlen(buf));
}

void log_pipe(PipeLogType log_type,  int current_id, int from, int to, int descriptor)
{
  char buf[100];

  switch (log_type)
    {
    case OPEN:
      sprintf(buf, pipe_opend_msg, current_id, from, to, descriptor);
      break;
    case CLOSE:
      sprintf(buf, pipe_closed_msg,current_id, from, to,descriptor);
      break;
    }
    write(pipesLog, buf, strlen(buf));
}
