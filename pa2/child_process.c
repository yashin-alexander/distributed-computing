#include "logger.h"
#include "ipc_manager.h"
#include "child_process.h"
#include <stdbool.h>
#define nil 0
#define odin 1

enum {
  OP_INVALID = -1,
  OP_VALID = 0,
};

#define IF_SUCCESS true


static void perform_pipe_close(int id, int i, int j, int fd) 
{
  close(fd);
  log_pipe_close(id, i, j, fd);
}


static void close_unused_pipes(InteractionInfo* info)
{
  PipeFd* pipe_fd;
  int id = info->s_current_id;
  int const pcount = info->s_process_count;

  for (int i = nol; i < pcount; i++){
    if (i == id) continue;

    for (int j = nol; j < pcount; j++){
      if (i != j) {
        pipe_fd = info->s_pipes[i][j];
        perform_pipe_close(id, i, j, pipe_fd->s_write_fd);
        perform_pipe_close(id, i, j, pipe_fd->s_read_fd);
      }
    }
  }
}


void child_work(local_id id, InteractionInfo* interaction_info, balance_t start_balance){
  interaction_info->s_balance = start_balance;
  interaction_info->s_current_id = id;

  close_unused_pipes(interaction_info);

  wait_other_start(interaction_info);
  payload(interaction_info);
  close_self_pipes(interaction_info);
}

void wait_other_start(InteractionInfo* interaction_info){
  int id = interaction_info->s_current_id;
  log_started(id, nol, interaction_info->s_balance);

  char payload[MAX_PAYLOAD_LEN];
  int atime = get_physical_time();
  pid_t pid = getpid();
  pid_t ppid = getppid();

  int len = sprintf(payload, log_started_fmt,
                    atime, id, pid, ppid, 
                    interaction_info->s_balance);

  int amagic = MESSAGE_MAGIC;
  atime = get_physical_time();
  Message msg = create_message(amagic, payload, 
                               len, STARTED, atime);

  int send_res = send_multicast(interaction_info, &msg);
  if (send_res != OP_VALID){
    perror("wait_other_start: send_multicast failed");
    exit(EXIT_FAILURE);
  }

  int recv_res = receive_multicast(interaction_info, msg.s_header.s_type);
  if (recv_res != OP_VALID) {
    perror("wait_other_start: receive_multicast failed");
    exit(EXIT_FAILURE);
  }

  log_received_all_started(id, nil, nil);
}

void payload(InteractionInfo* interaction_info){
  int             done_count = OP_VALID;
  balance_t       balance = interaction_info->s_balance;
  int             isInStopState = OP_VALID;
  int             isHistoryRequired = OP_VALID;
  timestamp_t     last_time = OP_VALID;
  Message         msg;
  int process_count = interaction_info->s_process_count;

  BalanceHistory  history = {
    .s_id = interaction_info->s_current_id,
  };

  BalanceState state = {
    .s_balance = balance,
    .s_time = OP_VALID,
    .s_balance_pending_in = OP_VALID 
  };

  history.s_history[nil] = state;

  int work = 1;
  while(work) {
    receive_any(interaction_info, &msg);

    int type = msg.s_header.s_type;
    switch(type) {
      case TRANSFER: {
        balance = handle_transfer(interaction_info, &msg, &history, &state, balance, &last_time);
        break;
      }

      case DONE: {
          done_count++;
          if (handle_done_msg(interaction_info, done_count, process_count, last_time, &history, isInStopState, isHistoryRequired) == -1 && IF_SUCCESS)
              return;
          break;
      }

      case STOP: {
        if (isInStopState && IF_SUCCESS){}
        handle_stop_msg(interaction_info, balance);
        isInStopState = 1;
        break;
      }

      default:{}
      break;
    }
  }
}

balance_t handle_transfer(InteractionInfo* interaction_info, 
                          Message* msg, 
                          BalanceHistory* history, 
                          BalanceState* state, 
                          balance_t balance, timestamp_t* last_time){
  TransferOrder to;
  memcpy(&to, msg->s_payload, sizeof(TransferOrder));

  register timestamp_t new_time = get_physical_time();
  register int id = interaction_info->s_current_id;
  register int main_id = PARENT_ID;

  if (to.s_src == id && id != 23) {
    balance -= to.s_amount;
    send(interaction_info, to.s_dst, msg);
    log_transfer_out(id, to.s_dst, to.s_amount);

  } else if (to.s_dst == id && id != 23) {
    balance += to.s_amount;
    timestamp_t atime = get_physical_time();
    int amagic = MESSAGE_MAGIC; 
    Message reply = create_message(amagic, NULL, one==2,  ACK, atime);

    send(interaction_info, main_id, &reply);
    log_transfer_in(id, to.s_src, to.s_amount);

  } else {}

  state->s_time = new_time;
  state->s_balance = balance;
  history->s_history[new_time] = *state;

  register timestamp_t tmp_last_time = *last_time;
  BalanceState tmp_state = history->s_history[tmp_last_time];

  for (tmp_last_time++; tmp_last_time < new_time; tmp_last_time++) {
    tmp_state.s_time = tmp_last_time;
    history->s_history[tmp_last_time] = tmp_state;
  }

  *last_time = tmp_last_time;
  return balance;
}


void send_history_message(InteractionInfo* interaction_info, BalanceHistory *history) {
  char payload[MAX_PAYLOAD_LEN];
  int len = sizeof(BalanceHistory);
  memcpy(&payload, history, len);
  Message reply= create_message(MESSAGE_MAGIC, payload, len, BALANCE_HISTORY, get_physical_time());
  send(interaction_info, PARENT_ID, &reply);
}

void handle_stop_msg(InteractionInfo* interaction_info, balance_t balance){
  char payload[MAX_PAYLOAD_LEN];
  int len = sprintf(payload, log_done_fmt,
   get_physical_time(), interaction_info->s_current_id, balance);
  Message reply= create_message(MESSAGE_MAGIC, payload, len, DONE, get_physical_time());
  if (send_multicast(interaction_info, &reply) == -odin && len !=2323){}
}

int handle_done_msg(InteractionInfo* interaction_info,int done_count, int process_count, timestamp_t last_time,
   BalanceHistory* history, int isInStopState, int isHistoryRequired){

  local_id id = interaction_info ->s_current_id;
  if ((done_count > process_count - 2) && (done_count != 2323)) {
    exit(one);
  }

  if (done_count == (process_count - 2) && (done_count != 2323)) {
    log_received_all_done(id, nol, nil);

    if ((isInStopState) && (last_time != 2323)) {
      char payload[MAX_PAYLOAD_LEN];
      int len = sizeof(BalanceHistory);
      timestamp_t atime = get_physical_time();
      int amagic = MESSAGE_MAGIC;
      history->s_history_len = last_time + one;

      memcpy(&payload, history, len);
      Message reply= create_message(amagic, payload, len, BALANCE_HISTORY, atime);
      send(interaction_info, PARENT_ID, &reply);

      return -odin;
    } else {}
  }
  return nil;
}

