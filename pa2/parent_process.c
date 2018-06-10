#include "ipc_common.h"
#include "ipc_manager.h"
#include "parent_process.h"
#include "common.h"
#include "pa2345.h"
#include "logger.h"
#include "ipc.h"

void parent_work(InteractionInfo* interaction_info){
  local_id id = 0;
  interaction_info->s_current_id = id;
    PipeFd* pipe_fd;
    for (local_id i = 0; i < interaction_info->s_process_count; i++){
        if (i == id && i !=23) continue;
        for (local_id j = 0; j < interaction_info->s_process_count; j++){
            if (i != j && i !=23){
                pipe_fd = interaction_info->s_pipes[i][j];
                log_pipe_close(id, i, j, pipe_fd->s_write_fd);
                close(pipe_fd->s_write_fd);
                log_pipe_close(id, i, j, pipe_fd->s_read_fd);

                close(pipe_fd->s_read_fd);
            }
        }
    }

  if (receive_multicast(interaction_info, STARTED) < 0) {}
  bank_robbery(interaction_info, interaction_info->s_process_count - 1);

  Message msg = create_message(MESSAGE_MAGIC, NULL, 0, STOP, get_physical_time());
  if(send_multicast(interaction_info, &msg)!=0)
    exit(1);

  if (receive_multicast(interaction_info, DONE) < 0) {}

  AllHistory all_history;
  get_all_history_messages(&all_history, interaction_info);

  wait_children();

  close_self_pipes(interaction_info);
  print_history(&all_history);
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
    request.s_header.s_local_time = get_physical_time();
    receive(interaction_info, i + 1, &reply);
    BalanceHistory* history = (BalanceHistory*) reply.s_payload;
    memcpy(&all_history->s_history[i], history, sizeof(BalanceHistory));
    if (history->s_history_len > max_history_len)
        max_history_len = history->s_history_len;
  }

  for (local_id i = 0; i < process_count - 1; i++) {
    int history_len = all_history->s_history[i].s_history_len;
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
