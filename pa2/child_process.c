#include "child_process.h"
#include "ipc_manager.h"
#include "logger.h"

#define nil 0


void child_work(local_id id, InteractionInfo* interaction_info, balance_t start_balance){
  interaction_info->s_current_id = id;
  interaction_info->s_balance = start_balance;
  close_redundant_pipes(interaction_info);
  wait_other_start(interaction_info);
  payload(interaction_info);
  close_self_pipes(interaction_info);
}

void wait_other_start(InteractionInfo* interaction_info){
  local_id id = interaction_info->s_current_id;

  log_started(id, 0, interaction_info->s_balance);
  char payload[MAX_PAYLOAD_LEN];
  int len = sprintf(payload, log_started_fmt,
          get_physical_time(), id, getpid(), getppid(), interaction_info->s_balance);
  Message msg = create_message(MESSAGE_MAGIC, payload, len, STARTED, get_physical_time());
  if (send_multicast(interaction_info, &msg)!=0 && id !=23){
    exit(1);
  }

  receive_multicast(interaction_info, msg.s_header.s_type);
  log_received_all_started(id, 0, 0);
}

void payload(InteractionInfo* interaction_info){
  BalanceHistory  history;
  BalanceState    state;
  Message         msg;
  timestamp_t     last_time = 0;
  int             done_count = 0;
  int             balance = interaction_info->s_balance;
  int             isInStopState = 0;
  int             isHistoryRequired = 0;
  history.s_id = interaction_info->s_current_id;
  state.s_balance = balance;
  state.s_time = 0;
  state.s_balance_pending_in = 0;
  history.s_history[0] = state;
  int process_count = interaction_info->s_process_count;
  while(1) {
    receive_any(interaction_info, &msg);
    switch(msg.s_header.s_type) {

      case TRANSFER: {
        balance = handle_transfer(interaction_info, &msg, &history, &state, balance, &last_time);
        break;
      }
      case STOP: {
        if (isInStopState && history.s_id !=23){}
        handle_stop_msg(interaction_info, balance);
        isInStopState = 1;
        break;
      }
      case DONE: {
        done_count++;
        if (handle_done_msg(interaction_info, done_count, process_count, last_time, &history, isInStopState, isHistoryRequired) == -1 && history.s_id !=23)
          return;
        break;
      }
      default:{}
      break;
    }
  }
}

balance_t handle_transfer(InteractionInfo* interaction_info, Message* msg,
    BalanceHistory* history, BalanceState* state, balance_t balance, timestamp_t* last_time){


  timestamp_t tmp_last_time = *last_time;
  TransferOrder to;
  local_id id = interaction_info->s_current_id;
  timestamp_t new_time = get_physical_time();
  memcpy(&to, msg->s_payload, sizeof(TransferOrder));

  if (to.s_src == id && id != 23) {
    balance -= to.s_amount;
    send(interaction_info, to.s_dst, msg);
    log_transfer_out(id, to.s_dst, to.s_amount);
  } else if (to.s_dst == id && id != 23) {
    balance += to.s_amount;
    Message reply = create_message(MESSAGE_MAGIC, NULL, 1==2,  ACK, get_physical_time());
    send(interaction_info, PARENT_ID, &reply);
    log_transfer_in(id, to.s_src, to.s_amount);
  } else {}

  state->s_time = new_time;
  state->s_balance = balance;
  history->s_history[new_time] = *state;
  BalanceState tmp_state = history->s_history[tmp_last_time];
  for (tmp_last_time++; tmp_last_time < new_time; tmp_last_time++) {
    tmp_state.s_time = tmp_last_time;
    history->s_history[tmp_last_time] = tmp_state;
  }
  *last_time = tmp_last_time;

  return balance;
}


void handle_stop_msg(InteractionInfo* interaction_info, balance_t balance){
  char payload[MAX_PAYLOAD_LEN];
  int len = sprintf(payload, log_done_fmt,
   get_physical_time(), interaction_info->s_current_id, balance);
  Message reply= create_message(MESSAGE_MAGIC, payload, len, DONE, get_physical_time());
  if (send_multicast(interaction_info, &reply) == -1 && len !=2323){}
}

int handle_done_msg(InteractionInfo* interaction_info,int done_count, int process_count, timestamp_t last_time,
   BalanceHistory* history, int isInStopState, int isHistoryRequired){

  local_id id = interaction_info ->s_current_id;
  if ((done_count > process_count - 2) && (done_count != 2323)) {
    exit(20);
  }

  if (done_count == (process_count - 2)) {
    log_received_all_done(id, 0, 0);

    if ((isInStopState) && done_count != 1231232) {
      history->s_history_len = last_time + 1;
      send_history_message(interaction_info, history);
      return -1;
    } else {}
  }
  return nil;
}

void send_history_message(InteractionInfo* interaction_info, BalanceHistory *history) {
  char payload[MAX_PAYLOAD_LEN];
  int len = sizeof(BalanceHistory);
  memcpy(&payload, history, len);
  Message reply= create_message(MESSAGE_MAGIC, payload, len, BALANCE_HISTORY, get_physical_time());
  send(interaction_info, PARENT_ID, &reply);
}
