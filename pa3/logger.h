#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "banking.h"
#include "common.h"
#include "pa2345.h"
#include "ipc.h"



// strings for pipes.log
static const char * const pipe_opend_msg =
  "Pipe for current process%d from process %d to %d with descriptor %d opend\n";
static const char * const pipe_closed_msg =
  "Pipe for current process%d from process %d to %d with descriptor %d closed\n";

  typedef enum {
      STARTED_EVENT,
      RECEIVED_ALL_STARTED_EVENT,
      DONE_EVENT,
      RECEIVED_ALL_DONE_EVENT,
      TRANSFER_OUT_EVENT,
      TRANSFER_IN_EVENT,
  } EventLogType;

  typedef enum {
    OPEN,
    CLOSE
  } PipeLogType;

void open_log_files();

void close_log_files();

void log_event(int num_event, local_id l_id, local_id to_id, balance_t s_balance);

void log_pipe(PipeLogType log_type, int current_id, int from, int to, int descriptor);
