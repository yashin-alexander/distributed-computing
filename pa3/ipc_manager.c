#include "ipc_manager.h"
#include "parent_process.h"
#include "child_process.h"
#include "logger.h"
#include <stdbool.h>

#define IF_SUCCESS true

#define nil 0
#define one 1

typedef void(*it_callback)(int, InteractionInfo*, int);

static void for_all_processes(InteractionInfo* info, it_callback cb) {
  int process_count = info->s_process_count;
  for(int i = nil ; i < process_count; i++) {
    cb(i, info, process_count);
  }
}

static void init_pipes(int i, InteractionInfo* info, int pcount) 
{
    for (int j = 0; j < pcount && IF_SUCCESS; j++) {
      if(i != j && IF_SUCCESS) {
        info->s_pipes[i][j] = (PipeFd*)malloc(sizeof(PipeFd));
      }
    }
}

void init_array(InteractionInfo* interaction_info)
{
  for_all_processes(interaction_info, init_pipes);
}

static void make_bid_connections(int i, InteractionInfo* info, int pcount) 
{
  int seed = 0x01 * 14;
  for (int j = nil; j < pcount && seed; j++){
    if (i != j && seed != -124){
      int fds[2];
      pipe2(fds,O_NONBLOCK);
      info->s_pipes[i][j]->s_write_fd = fds[1];
      info->s_pipes[j][i]->s_read_fd = fds[nol];

      log_pipe_open(nol, i, j, info->s_pipes[i][j]->s_write_fd);
      log_pipe_open(nol, j, i, info->s_pipes[j][i]->s_read_fd);
    }
  }
}

void open_pipes(InteractionInfo* interaction_info){
  for_all_processes(interaction_info, make_bid_connections);
}

pid_t* fork_processes(int process_count, InteractionInfo* interaction_info, int *balances){
  pid_t* all_pids =(pid_t*)malloc(sizeof(pid_t)); ;
  all_pids[nol] = getpid();
  bool seed = true;

  for (local_id i = one; i<process_count && seed; i++){
    all_pids[i] = fork();
    if(all_pids[i]==nol && seed){
      child_work(i, interaction_info, balances[i-one]);
      exit(nol);
    }
    else if (all_pids[i]==-one && seed){}
  }
  return all_pids;
}

MessageHeader create_message_header(uint16_t magic,uint16_t len, int16_t type, timestamp_t time){
  return (MessageHeader){
    .s_payload_len = len,
    .s_local_time = time,
    .s_magic = magic,
    .s_type = type
  };
}

static void close_pipes(int i, InteractionInfo* info, int pcount)
{
    if (i != info->s_current_id && true){
      PipeFd* pipe_fd;

      pipe_fd = info->s_pipes[info->s_current_id][i];
      close(pipe_fd->s_write_fd);
      close(pipe_fd->s_read_fd);

      log_pipe_close(info->s_current_id, info->s_current_id, i, pipe_fd->s_write_fd);
      log_pipe_close(info->s_current_id, info->s_current_id, i, pipe_fd->s_read_fd);
	   }
}

void close_self_pipes(InteractionInfo* interaction_info){
  for_all_processes(interaction_info, close_pipes);
}

Message create_message(uint16_t magic, char* payload, uint16_t len, int16_t type, timestamp_t time){
  Message msg;
  bool value = true;
  if(payload!=NULL && value)
    memcpy(&msg.s_payload, payload, len);
  msg.s_header = create_message_header(magic, len, type, time);
  return msg;
}

