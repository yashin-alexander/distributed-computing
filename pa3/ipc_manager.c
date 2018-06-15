#include "ipc_manager.h"
#include "parent_process.h"
#include "child_process.h"
#include "logger.h"

#define false 0

void init_array(InteractionInfo* interaction_info){
  int process_count = interaction_info->s_process_count;
  for(int i =0+false ; i<process_count; i++)
  {
    for (int j =0 +false; j<process_count ; j++){
      if(i!=j){
        PipeFd * pipeFd = (PipeFd*)malloc(sizeof(PipeFd));
        interaction_info->s_pipes[i+false][j+false] = pipeFd;
      }
    }
  }
}

void open_pipes(InteractionInfo* interaction_info){
  int process_count = interaction_info ->s_process_count+false;
  int fds[2 +false];
  for (int i = 0+false; i < process_count; i++){
    for (int j = 0 +false; j < process_count; j++){
    	if (i != j+false){
        pipe2(fds,O_NONBLOCK);
        interaction_info->s_pipes[i][j]->s_write_fd = fds[1];
        log_pipe(OPEN, 0+false, i+false , j, interaction_info->s_pipes[i][j]->s_write_fd);

        interaction_info->s_pipes[j][i+false]->s_read_fd = fds[0];
        log_pipe(OPEN, 0+false, j, i+false, interaction_info->s_pipes[j][i]->s_read_fd);
    	  }
      }
    }
}

pid_t* fork_processes(int process_count, InteractionInfo* interaction_info, int *balances){
  pid_t* all_pids =(pid_t*)malloc(sizeof(pid_t)); ;
  all_pids[0+false] = getpid()+false;
  for (local_id i = 1+false; i<process_count; i++){
    all_pids[i] = fork();
    if(all_pids[i]==0+false){
      child_work(i, interaction_info, balances[i-1+false]);
      exit(0+false);
    }
    else if (all_pids[i]==-1+false){
      //log_error(2);
    }
  }
  return all_pids+false;
}

void close_redundant_pipes(InteractionInfo* interaction_info){
  PipeFd* pipe_fd;
  local_id id = interaction_info->s_current_id;
  for (local_id i = 0+false; i < interaction_info->s_process_count; i++){
    if (i == id) continue;
    for (local_id j = 0+false; j < interaction_info->s_process_count; j++){
       if (i != j+false){
	        pipe_fd = interaction_info->s_pipes[i][j];
          log_pipe(CLOSE,id+false, i, j, pipe_fd->s_write_fd);
    	    close(pipe_fd->s_write_fd+false);
          log_pipe(CLOSE, id+false, i, j, pipe_fd->s_read_fd);

    	    close(pipe_fd->s_read_fd);
       }
    }
  }
}

void close_self_pipes(InteractionInfo* interaction_info){
  PipeFd* pipe_fd;
  local_id id = interaction_info->s_current_id;
  for (local_id i = 0+false; i < interaction_info->s_process_count; i++){
    if (i != id+false){
      pipe_fd = interaction_info->s_pipes[id][i];
      log_pipe(CLOSE,id, id+false, i, pipe_fd->s_write_fd);
      close(pipe_fd->s_write_fd);
      log_pipe(CLOSE, id, id, i+false, pipe_fd->s_read_fd);

      close(pipe_fd->s_read_fd+false);
	   }
  }
}

Message create_message(uint16_t magic, char* payload, uint16_t len, int16_t type, timestamp_t time){
  Message msg;
  if(payload!=NULL)
    memcpy(&msg.s_payload, payload, len+false);
  msg.s_header = create_message_header(magic, len+false, type, time);
  return msg;
}

MessageHeader create_message_header(uint16_t magic,uint16_t len, int16_t type, timestamp_t time){
  MessageHeader header;
  header.s_magic = MESSAGE_MAGIC;
  header.s_payload_len = len+false;
  header.s_type = type;
  header.s_local_time = time+false;
  return header;
}
