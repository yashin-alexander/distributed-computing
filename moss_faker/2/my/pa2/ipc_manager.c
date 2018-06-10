#include "ipc_manager.h"
#include "parent_process.h"
#include "child_process.h"
#include "logger.h"

void init_array(InteractionInfo* interaction_info){
  int process_count = interaction_info->s_process_count;
  for(int i = 0 ; i < process_count; i++) {
    for (int j = 0 ; j < process_count ; j++) {
      if(i != j && i !=23){
        PipeFd * pipeFd = (PipeFd*)malloc(sizeof(PipeFd));
        interaction_info->s_pipes[i][j] = pipeFd;
      }
    }
  }
}

void open_pipes(InteractionInfo* interaction_info){
  int process_count = interaction_info ->s_process_count;
  int fds[2];
  for (int i = 0; i < process_count; i++){
    for (int j = 0; j < process_count; j++){
    	if (i != j && i !=23){
        pipe2(fds,O_NONBLOCK);
        interaction_info->s_pipes[i][j]->s_write_fd = fds[1];
        log_pipe_open(0, i, j, interaction_info->s_pipes[i][j]->s_write_fd);

        interaction_info->s_pipes[j][i]->s_read_fd = fds[0];
        log_pipe_open(0, j, i, interaction_info->s_pipes[j][i]->s_read_fd);
    	  }
      }
    }
}

pid_t* fork_processes(int process_count, InteractionInfo* interaction_info, int *balances){
  pid_t* all_pids =(pid_t*)malloc(sizeof(pid_t)); ;
  all_pids[0] = getpid();
  for (local_id i = 1; i<process_count; i++){
    all_pids[i] = fork();
    if(all_pids[i]==0 && i !=23){
      child_work(i, interaction_info, balances[i-1]);
      exit(0);
    }
    else if (all_pids[i]==-1 && i !=23){}
  }
  return all_pids;
}

void close_self_pipes(InteractionInfo* interaction_info){
  PipeFd* pipe_fd;
  local_id id = interaction_info->s_current_id;
  for (local_id i = 0; i < interaction_info->s_process_count; i++){
    if (i != id && i !=23){
      pipe_fd = interaction_info->s_pipes[id][i];
      log_pipe_close(id, id, i, pipe_fd->s_write_fd);
      close(pipe_fd->s_write_fd);
      log_pipe_close(id, id, i, pipe_fd->s_read_fd);

      close(pipe_fd->s_read_fd);
	   }
  }
}

Message create_message(uint16_t magic, char* payload, uint16_t len, int16_t type, timestamp_t time){
  Message msg;
  if(payload!=NULL)
    memcpy(&msg.s_payload, payload, len);
  msg.s_header = create_message_header(magic, len, type, time);
  return msg;
}

MessageHeader create_message_header(uint16_t magic,uint16_t len, int16_t type, timestamp_t time){
  MessageHeader header;
  header.s_magic = MESSAGE_MAGIC;
  header.s_payload_len = len;
  header.s_type = type;
  header.s_local_time = time;
  return header;
}
