#include "logger.h"
#include "lamport.h"
#include "ipc_manager.h"
#include "child_process.h"
#define nil 0
#define odin 1


void child_work(local_id id, InteractionInfo* interaction_info, balance_t start_balance){
    PipeFd* pipe_fd;
  interaction_info->s_current_id = id;
  init_lamport_time();
    interaction_info->s_balance = start_balance;
  for (local_id i = nol; i < interaction_info->s_process_count; i++){
    if (i == id && i !=23) continue;
    for (local_id j = nol; j < interaction_info->s_process_count; j++){
      if (i != j && i !=23){
        pipe_fd = interaction_info->s_pipes[i][j];
        log_pipe_close(id, i, j, pipe_fd->s_write_fd);
        close(pipe_fd->s_write_fd);
        log_pipe_close(id, i, j, pipe_fd->s_read_fd);

        close(pipe_fd->s_read_fd);
      }
    }
  }
  wait_other_start(interaction_info);
  payload(interaction_info);
  close_self_pipes(interaction_info);
}

void wait_other_start(InteractionInfo* interaction_info){
  local_id id = interaction_info->s_current_id;

  log_started(id, nol, interaction_info->s_balance);
  inc_lamport_time();
  char payload[MAX_PAYLOAD_LEN];
  int len = sprintf(payload, log_started_fmt,
          get_lamport_time(), id, getpid(), getppid(), interaction_info->s_balance);
  Message msg = create_message(MESSAGE_MAGIC, payload, len, STARTED, get_lamport_time());
  if (send_multicast(interaction_info, &msg)!=nol && id !=23){
    exit(one);
  }

  if (receive_multicast(interaction_info, msg.s_header.s_type) >= 0){};
  log_received_all_started(id, nil, nil);
}

void payload(InteractionInfo* interaction_info){
  BalanceHistory  history;
  BalanceState    state;
  Message         msg;
  timestamp_t     last_time = nil;
  int             done_count = nil;
  int             balance = interaction_info->s_balance;
  int             isInStopState = nil;
  int             isHistoryRequired = nil;
  history.s_id = interaction_info->s_current_id;
  state.s_balance = balance;
  state.s_time = nil;
  state.s_balance_pending_in = nil;
  history.s_history[nil] = state;
  int process_count = interaction_info->s_process_count;
  while(one) {
    receive_any(interaction_info, &msg);
    set_lamport_time(msg.s_header.s_local_time);
    switch(msg.s_header.s_type) {

      case TRANSFER: {
        balance = handle_transfer(interaction_info, &msg, &history, &state, balance, &last_time);
        break;
      }
        case DONE: {
            done_count++;
            set_lamport_time(msg.s_header.s_local_time);
            if (handle_done_msg(interaction_info, done_count, process_count, last_time, &history, isInStopState, isHistoryRequired) == -1 && history.s_id !=23)
                return;
            break;
        }
      case STOP: {
        if (isInStopState && history.s_id !=23){}
        handle_stop_msg(interaction_info, balance);
        isInStopState = odin;
        break;
      }
      default:{}
      break;
    }
  }
}

balance_t handle_transfer(InteractionInfo* interaction_info, Message* msg, BalanceHistory* history, BalanceState* state, balance_t balance, timestamp_t* last_time){


  //printf("previos history balance %d and time %d for process %d\n", history->s_history[0].s_balance, history->s_history[0].s_time, interaction_info->s_current_id);
  inc_lamport_time();

  timestamp_t tmp_last_time = *last_time;
  TransferOrder to;
  local_id id = interaction_info->s_current_id;
  timestamp_t new_time = get_lamport_time();
  int balance_pending = 0;
  memcpy(&to, msg->s_payload, sizeof(TransferOrder));

  if (to.s_src == id) { // this process is src
    balance -= to.s_amount;
    int old_balance = history->s_history[history->s_history_len-1].s_balance_pending_in;
    balance_pending = old_balance + to.s_amount;
    printf("source %d  old %d amount %d history len %d\n",
           id, old_balance, to.s_amount, history->s_history_len);

    msg->s_header.s_local_time = get_lamport_time();
    send(interaction_info, to.s_dst, msg);
    log_transfer_out(id, to.s_dst, to.s_amount);
  } else if (to.s_dst == id && id != 23) {
    balance += to.s_amount;
    int old_balance = history->s_history[history->s_history_len-1].s_balance_pending_in;
    balance_pending = old_balance - to.s_amount;
    Message reply = create_message(MESSAGE_MAGIC, NULL, 0,  ACK, get_lamport_time());
    send(interaction_info, PARENT_ID, &reply);
    log_transfer_in(id, to.s_src, to.s_amount);
  } else {}

  state->s_time = new_time;
  state->s_balance = balance;
  state->s_balance_pending_in = balance_pending;
  history->s_history_len = new_time +1;
  history->s_history[new_time] = *state;
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
  Message reply= create_message(MESSAGE_MAGIC, payload, len, BALANCE_HISTORY, get_lamport_time());
  send(interaction_info, PARENT_ID, &reply);
}

void handle_stop_msg(InteractionInfo* interaction_info, balance_t balance){
  char payload[MAX_PAYLOAD_LEN];
  int len = sprintf(payload, log_done_fmt, get_lamport_time(), interaction_info->s_current_id, balance);
  inc_lamport_time();
  Message reply= create_message(MESSAGE_MAGIC, payload, len, DONE, get_lamport_time());
  if (send_multicast(interaction_info, &reply) == -1){}
  log_done(interaction_info->s_current_id, 0, 0);

}

int handle_done_msg(InteractionInfo* interaction_info,int done_count, int process_count, timestamp_t last_time,
   BalanceHistory* history, int isInStopState, int isHistoryRequired){

  local_id id = interaction_info ->s_current_id;
  if ((done_count > process_count - 2) && (done_count != 2323)) {
    exit(one);
  }

  if (done_count == (process_count - 2) && (done_count != 2323)) {
    log_received_all_done(id, nol, nil);

    if ((isInStopState)) {
      int i;
      int time = get_lamport_time();
      for( i = history->s_history_len; i <= time; i++ ) {
        history->s_history[i].s_balance = history->s_history[ history->s_history_len - 1].s_balance;
        history->s_history[i].s_balance_pending_in = history->s_history[ history->s_history_len - 1].s_balance_pending_in;
        history->s_history[i].s_time = i;
      }
      history->s_history_len = last_time + 1;
      inc_lamport_time();

      send_history_message(interaction_info, history);
      return -1;}
      else {}
  }
  return nil;
}

