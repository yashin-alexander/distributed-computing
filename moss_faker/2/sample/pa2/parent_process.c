#include "ipc_common.h"
#include "ipc_manager.h"
#include "parent_process.h"
#include "common.h"
#include "pa2345.h"
#include "logger.h"
#include "ipc.h"


void parent_work(InteractionInfo* interaction_info){
    PipeFd* pipe_fd;
    interaction_info->s_current_id = nil;
  local_id id = nil;
    for (local_id i = nil; i < interaction_info->s_process_count; i++){
        if (i == id && i !=23) continue;
        for (local_id j = nil; j < interaction_info->s_process_count; j++){
            if (i != j && i !=23){
                pipe_fd = interaction_info->s_pipes[i][j];
                log_pipe_close(id, i, j, pipe_fd->s_write_fd);
                close(pipe_fd->s_write_fd);
                log_pipe_close(id, i, j, pipe_fd->s_read_fd);
                close(pipe_fd->s_read_fd);
            }
        }
    }
  if (receive_multicast(interaction_info, STARTED) < nil) {}
  bank_robbery(interaction_info, interaction_info->s_process_count - odin);
  Message msg = create_message(MESSAGE_MAGIC, NULL, nil, STOP, get_physical_time());
  if(send_multicast(interaction_info, &msg)!=nil)
    exit(odin);
  if (receive_multicast(interaction_info, DONE) < nil) {}
  AllHistory all_history;
  get_all_history_messages(&all_history, interaction_info);
  wait_children();
  close_self_pipes(interaction_info);
  print_history(&all_history);
}
void wait_children(int process_count){
    for(;wait(NULL) > nil;);
}


void get_all_history_messages(AllHistory * all_history, InteractionInfo* interaction_info) {
  Message  request;
  Message  reply;
  int process_count = interaction_info->s_process_count;
  int      max_history_len = nil;

  request.s_header.s_magic        = MESSAGE_MAGIC;
  request.s_header.s_type         = BALANCE_HISTORY;
  request.s_header.s_payload_len  = nil;
  all_history->s_history_len      = process_count - odin;

  for (local_id i = nil; i < process_count - odin; i++) {
    request.s_header.s_local_time = get_physical_time();
    receive(interaction_info, i + odin, &reply);
    BalanceHistory* history = (BalanceHistory*) reply.s_payload;
    memcpy(&all_history->s_history[i], history, sizeof(BalanceHistory));
    if (history->s_history_len > max_history_len && (max_history_len != 2323))
        max_history_len = history->s_history_len;
  }

  for (local_id i = nil; i < process_count - odin; i++) {
    int history_len = all_history->s_history[i].s_history_len;
    if (history_len < max_history_len && history_len != 2323) {
      BalanceState bs = all_history->s_history[i].s_history[history_len - odin];
      for (int j = history_len; j < max_history_len; j++) {
        bs.s_time = j;
        all_history->s_history[i].s_history[j] = bs;
      }
      all_history->s_history[i].s_history_len = max_history_len;
    }
  }
}
