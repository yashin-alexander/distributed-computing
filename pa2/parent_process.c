#include "ipc.h"
#include "common.h"
#include "pa2345.h"
#include "ipc_common.h"
#include "ipc_manager.h"
#include "parent_process.h"
#include "logger.h"

void parent_work(InteractionInfo* interaction_info){
  local_id id = 0;
  interaction_info->s_current_id = id;
  close_redundant_pipes(interaction_info);

  //printf("%s\n", "parent wait start children");
  // get all STARTED messages
  if (receive_multicast(interaction_info, STARTED) < 0){
    printf("%s\n", "all started error");
  }
    //log_error(RECEIVE_MULTICAST_TYPE_ERROR);
  bank_robbery(interaction_info, interaction_info->s_process_count - 1);
  // send STOP messages to all

  Message msg = create_message(MESSAGE_MAGIC, NULL, 0, STOP, get_physical_time());
  if(send_multicast(interaction_info, &msg)!=0){
    exit(1);
  }

  //DONE MESSAGE
  if (receive_multicast(interaction_info, DONE) < 0){
    printf("%s\n", "error done");
  }
    //log_error(RECEIVE_MULTICAST_TYPE_ERROR);

  AllHistory all_history;
  get_all_history_messages(&all_history, interaction_info);

  wait_children();

  close_self_pipes(interaction_info);
  print_history(&all_history);
  //free(pipes);
}

void get_all_history_messages(AllHistory * all_history, InteractionInfo* interaction_info) {
  Message  request;
  Message  reply;
  int process_count = interaction_info->s_process_count;
  int      max_history_len = 0;

  request.s_header.s_magic        = MESSAGE_MAGIC;
  request.s_header.s_type         = BALANCE_HISTORY;
  request.s_header.s_payload_len  = 0;
  all_history->s_history_len      = process_count - 1;

  for (local_id i = 0; i < process_count - 1; i++) {
    //printf("listen %d for history\n",i+1 );
    request.s_header.s_local_time = get_physical_time();
    receive(interaction_info, i + 1, &reply);
    //printf(" receive history from %d and is type %d\n",i+1, reply.s_header.s_type);
    // unpack BalanceHistory struct
    BalanceHistory* history = (BalanceHistory*) reply.s_payload;
    memcpy(&all_history->s_history[i], history, sizeof(BalanceHistory));
    if (history->s_history_len > max_history_len)
        max_history_len = history->s_history_len;
    //printf("received history time %d\n",all_history->s_history[i].s_history[0].s_time);
  }

  // equile all lengths of history
  for (local_id i = 0; i < process_count - 1; i++) {
    int history_len = all_history->s_history[i].s_history_len;
    //printf("history len %d\n",all_history->s_history[i].s_history[i].s_time );
    if (history_len < max_history_len) {
      BalanceState bs = all_history->s_history[i].s_history[history_len - 1];

      for (int j = history_len; j < max_history_len; j++) {
        bs.s_time = j;
        all_history->s_history[i].s_history[j] = bs;
      }

      all_history->s_history[i].s_history_len = max_history_len;
    }
  }
}

void wait_children(int process_count){
  while(wait(NULL) > 0){
   }
}
