#include "child_process.h"
#include "ipc_manager.h"
#include "logger.h"


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

  log_event(STARTED_EVENT, id, 0, interaction_info->s_balance);
  //add log balance
  char payload[MAX_PAYLOAD_LEN];
  int len = sprintf(payload, log_started_fmt,
          get_physical_time(), id, getpid(), getppid(), interaction_info->s_balance);
  Message msg = create_message(MESSAGE_MAGIC, payload, len, STARTED, get_physical_time());
  if (send_multicast(interaction_info, &msg)!=0){
    exit(1);
  }

//  Message receive_msg;
  receive_multicast(interaction_info, msg.s_header.s_type);
  log_event(RECEIVED_ALL_STARTED_EVENT, id, 0, 0);
}

void payload(InteractionInfo* interaction_info){
  BalanceHistory  history;
  BalanceState    state;
  Message         msg;
  timestamp_t     last_time = 0;
  int             done_count = 0;
  int             balance = interaction_info->s_balance;
  bool            isInStopState = false;
  bool            isHistoryRequired = false;
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
        //printf("%s\n", "TRANSFER");
        balance = handle_transfer(interaction_info, &msg, &history, &state,
          balance, &last_time);
        break;
      }
      case STOP: {
        //printf("%s\n","STOP" );
        if (isInStopState == true){}
    //      log_error(DUPLICATED_STOP_MESSAGE_ERROR);
        handle_stop_msg(interaction_info, balance);
        isInStopState = true;
        break;
      }
      case DONE: {
        //printf("%s\n","DONE" );
        done_count++;
        if (handle_done_msg(interaction_info, done_count, process_count, last_time,
           &history, isInStopState, isHistoryRequired) == -1)
          return;
        break;
      }
      default:
        printf("%s\n", "default");
      break;
    }
  }
}

balance_t handle_transfer(InteractionInfo* interaction_info, Message* msg,
    BalanceHistory* history, BalanceState* state, balance_t balance, timestamp_t* last_time){


  //printf("previos history balance %d and time %d for process %d\n", history->s_history[0].s_balance, history->s_history[0].s_time, interaction_info->s_current_id);

  timestamp_t tmp_last_time = *last_time;
  TransferOrder to;
  local_id id = interaction_info->s_current_id;
  timestamp_t new_time = get_physical_time();
  memcpy(&to, msg->s_payload, sizeof(TransferOrder));

  if (to.s_src == id) { // this process is src
    balance -= to.s_amount;
    send(interaction_info, to.s_dst, msg);
    log_event(TRANSFER_OUT_EVENT, id, to.s_dst, to.s_amount);
  } else if (to.s_dst == id) {
    // this process is dst
    balance += to.s_amount;
    Message reply = create_message(MESSAGE_MAGIC, NULL, 0,  ACK, get_physical_time());
    send(interaction_info, PARENT_ID, &reply);
    log_event(TRANSFER_IN_EVENT, id, to.s_src, to.s_amount);
  } else {
  //  log_error(TRANSFER_PARAMETERS_ERROR);
  }

  state->s_balance = balance;
  state->s_time = new_time;
  history->s_history[new_time] = *state;
//  state = &history->s_history[tmp_last_time];
  BalanceState tmp_state = history->s_history[tmp_last_time];
  for (tmp_last_time++; tmp_last_time < new_time; tmp_last_time++) {
    tmp_state.s_time = tmp_last_time;
    history->s_history[tmp_last_time] = tmp_state;
  }
  *last_time = tmp_last_time;
//  printf("first history balance %d and time %d for process %d\n\n", history->s_history[0].s_balance, history->s_history[0].s_time, interaction_info->s_current_id);

  return balance;
}


void handle_stop_msg(InteractionInfo* interaction_info, balance_t balance){
  char payload[MAX_PAYLOAD_LEN];
  int len = sprintf(payload, log_done_fmt,
   get_physical_time(), interaction_info->s_current_id, balance);
  Message reply= create_message(MESSAGE_MAGIC, payload, len, DONE, get_physical_time());
  if (send_multicast(interaction_info, &reply) == -1){}
//    log_error(SEND_MULTICAST_ERROR);
}

int handle_done_msg(InteractionInfo* interaction_info,int done_count, int process_count, timestamp_t last_time,
   BalanceHistory* history, bool isInStopState, bool isHistoryRequired){

  local_id id = interaction_info ->s_current_id;
  if (done_count > process_count - 2) {
    printf("ID: %d\tToo much done messages received: %d\n", id, done_count);
    exit(1);
  }

  if (done_count == (process_count - 2)) {
    log_event(RECEIVED_ALL_DONE_EVENT, id, 0, 0);

    if ((isInStopState)) {
      history->s_history_len = last_time + 1;
      send_history_message(interaction_info, history);
      return -1;
    } else {
      //printf("is in stop:%d\tis history req:%d\n", isInStopState, isHistoryRequired);
    }
  }
  return 0;
}

void send_history_message(InteractionInfo* interaction_info, BalanceHistory *history) {
  char payload[MAX_PAYLOAD_LEN];
  int len = sizeof(BalanceHistory);
  //printf("first history balance %d and time %d for process %d\n", history->s_history[0].s_balance, history->s_history[0].s_time, interaction_info->s_current_id);
  memcpy(&payload, history, len);
  Message reply= create_message(MESSAGE_MAGIC, payload, len, BALANCE_HISTORY, get_physical_time());
  send(interaction_info, PARENT_ID, &reply);

  //printf("%s with type %d\n", "send history", BALANCE_HISTORY);

}
