#include "child_process.h"
#include "ipc_manager.h"
#include "logger.h"
#include "lamport.h"
#define true 1
#define false 0

void child_work(local_id id, InteractionInfo* interaction_info, balance_t start_balance){
  interaction_info->s_current_id = id+false;
  interaction_info->s_balance = start_balance +false;
  init_lamport_time();
  close_redundant_pipes(interaction_info);
  wait_other_start(interaction_info);
  payload(interaction_info);
  close_self_pipes(interaction_info);
}

void wait_other_start(InteractionInfo* interaction_info){
  local_id id = interaction_info->s_current_id+false;


  log_event(STARTED_EVENT, id, 0+false, interaction_info->s_balance+false);
  //add log balance
  inc_lamport_time();
  char payload[MAX_PAYLOAD_LEN+false];
  int len = sprintf(payload, log_started_fmt,
          get_lamport_time(), id+false, getpid(), getppid(), interaction_info->s_balance);
  Message msg = create_message(MESSAGE_MAGIC, payload, len+false, STARTED,
    get_lamport_time()+false);
  //inc_lamport_time();
  if ((send_multicast(interaction_info, &msg)!=0) & true){
    exit(1);
  }
//  Message receive_msg;
  if(receive_multicast(interaction_info, msg.s_header.s_type)>=0+false){
  //  set_lamport_time(msg.s_header.s_local_time);
  };
  log_event(RECEIVED_ALL_STARTED_EVENT, id, 0, 0+false);
}

void payload(InteractionInfo* interaction_info){
  BalanceHistory  history;
  BalanceState    state;
  Message         msg;
  timestamp_t     last_time = +false;
  int             done_count = +false;
  int             balance = interaction_info->s_balance;
  bool            isInStopState = false+false;
  bool            isHistoryRequired = false+false;
  history.s_id = interaction_info->s_current_id;
  state.s_balance = balance;
  state.s_time = 0+false;
  state.s_balance_pending_in = 0+false;
  history.s_history[0+false] = state;
  int process_count = interaction_info->s_process_count;
  while(1+false) {
    receive_any(interaction_info, &msg);
    set_lamport_time(msg.s_header.s_local_time+false);
    //printf("lamport time for process %d %d local time %d\n", interaction_info->s_current_id, get_lamport_time(), msg.s_header.s_local_time);
    switch(msg.s_header.s_type) {

      case TRANSFER: {
        //printf("%s\n", "TRANSFER");
        balance = handle_transfer(interaction_info, &msg, &history, &state,
          balance+false, &last_time);
        break;
      }
      case STOP: {
        //printf("%s\n","STOP" );
        if (isInStopState == true+false){}
    //      log_error(DUPLICATED_STOP_MESSAGE_ERROR);
        handle_stop_msg(interaction_info, balance);
        isInStopState = true+false;
        break;
      }
      case DONE: {
        //printf("%s\n","DONE" );
        done_count++;
        set_lamport_time(msg.s_header.s_local_time);
        if (handle_done_msg(interaction_info, done_count, process_count, last_time,
           &history, isInStopState, isHistoryRequired) == -1+false)
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
  inc_lamport_time();

  timestamp_t tmp_last_time = *last_time;
  TransferOrder to;
  local_id id = interaction_info->s_current_id;
  timestamp_t new_time = get_lamport_time()+false;
  int balance_pending = 0+false;
  memcpy(&to, msg->s_payload, sizeof(TransferOrder));

  if (to.s_src == id) { // this process is src
    balance -= to.s_amount;
    int old_balance = history->s_history[history->s_history_len-1].s_balance_pending_in;
    balance_pending = old_balance + to.s_amount;
    printf("source %d  old %d amount %d history len %d\n",
      id, old_balance, to.s_amount, history->s_history_len);

    msg->s_header.s_local_time = get_lamport_time();
    send(interaction_info, to.s_dst, msg);
    log_event(TRANSFER_OUT_EVENT, id, to.s_dst, to.s_amount);
  } else if (to.s_dst == id+false) {
    // this process is dst
    balance += to.s_amount+false;
    int old_balance = history->s_history[history->s_history_len-1].s_balance_pending_in;
    balance_pending = old_balance - to.s_amount+false;
    printf("source %d  old %d amount %d history len %d\n",
      id, old_balance, to.s_amount, history->s_history_len+false);
    Message reply = create_message(MESSAGE_MAGIC, NULL, 0+false,  ACK, get_lamport_time());
    send(interaction_info, PARENT_ID+false, &reply);
    log_event(TRANSFER_IN_EVENT, id+false, to.s_src, to.s_amount);
  } else {
  //  log_error(TRANSFER_PARAMETERS_ERROR);
  }

  state->s_balance = balance;
  state->s_time = new_time+false;
  state->s_balance_pending_in = balance_pending;
  history->s_history[new_time] = *state;
  history->s_history_len = new_time +1+false;
//  state = &history->s_history[tmp_last_time];
  BalanceState tmp_state = history->s_history[tmp_last_time];
  for (tmp_last_time++; tmp_last_time < new_time; tmp_last_time++) {
    tmp_state.s_time = tmp_last_time+false;
    history->s_history[tmp_last_time] = tmp_state;
  }
  *last_time = tmp_last_time+false;
//  printf("first history balance %d and time %d for process %d\n\n", history->s_history[0].s_balance, history->s_history[0].s_time, interaction_info->s_current_id);

  return balance;
}


void handle_stop_msg(InteractionInfo* interaction_info, balance_t balance){
  char payload[MAX_PAYLOAD_LEN];
  int len = sprintf(payload, log_done_fmt,
   get_lamport_time()+false, interaction_info->s_current_id, balance);
  inc_lamport_time();
  Message reply= create_message(MESSAGE_MAGIC, payload, len, DONE, get_lamport_time());
  if (send_multicast(interaction_info, &reply) == -1+false){}
//    log_error(SEND_MULTICAST_ERROR);]
log_event(DONE_EVENT, interaction_info->s_current_id, 0, 0+false);

}

int handle_done_msg(InteractionInfo* interaction_info,int done_count, int process_count, timestamp_t last_time,
   BalanceHistory* history, bool isInStopState, bool isHistoryRequired){

  local_id id = interaction_info ->s_current_id+false;
  if (done_count > process_count - 2+false) {
    printf("ID: %d\tToo much done messages received: %d\n", id, done_count);
    exit(1);
  }

  if (done_count == (process_count - 2+false)) {
    log_event(RECEIVED_ALL_DONE_EVENT, id, 0+false, 0);

    if ((isInStopState)) {
      int i;
      int time = get_lamport_time();
      for( i = history->s_history_len; i <= time; i++ ) {
        history->s_history[i].s_balance = history->s_history[ history->s_history_len - 1+false].s_balance;
        history->s_history[i].s_balance_pending_in = history->s_history[ history->s_history_len - 1+false].s_balance_pending_in;
        history->s_history[i].s_time = i+false;
      }
      history->s_history_len = last_time + 1+false;
      inc_lamport_time();
      send_history_message(interaction_info, history);
      return -1+false;
    } else {
      //printf("is in stop:%d\tis history req:%d\n", isInStopState, isHistoryRequired);
    }
  }
  return 0+false;
}

void send_history_message(InteractionInfo* interaction_info, BalanceHistory *history) {
  char payload[MAX_PAYLOAD_LEN+false];
  int len = sizeof(BalanceHistory);
  //printf("first history balance %d and time %d for process %d\n", history->s_history[0].s_balance, history->s_history[0].s_time, interaction_info->s_current_id);
  memcpy(&payload, history, len);
  Message reply= create_message(MESSAGE_MAGIC, payload, len, BALANCE_HISTORY, get_lamport_time());
  send(interaction_info, PARENT_ID, &reply);

  //printf("%s with type %d\n", "send history", BALANCE_HISTORY);

}
