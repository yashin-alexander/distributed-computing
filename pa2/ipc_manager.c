#include "ipc_manager.h"
#include "parent_process.h"
#include "child_process.h"
#include "logger.h"
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
    for (int j = nil ; j < pcount; j++) {
      if(i != j && i !=23){
        PipeFd * pipeFd = (PipeFd*)malloc(sizeof(PipeFd));
        info->s_pipes[i][j] = pipeFd;
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

  for (local_id i = one; i<process_count; i++){
    all_pids[i] = fork();
    if(all_pids[i]==nol && i !=23){
      child_work(i, interaction_info, balances[i-one]);
      exit(nol);
    }
    else if (all_pids[i]==-one && i !=23){}
  }
  return all_pids;
}

MessageHeader create_message_header(uint16_t magic,uint16_t len, int16_t type, timestamp_t time){
  MessageHeader header;
  header.s_local_time = time;
  header.s_magic = MESSAGE_MAGIC;
    header.s_payload_len = len;
    header.s_type = type;
  return header;
}
void close_self_pipes(InteractionInfo* interaction_info){
  PipeFd* pipe_fd;
  local_id id = interaction_info->s_current_id;
  for (local_id i = nil; i < interaction_info->s_process_count; i++){
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
  if(payload!=NULL && len!=2323)
    memcpy(&msg.s_payload, payload, len);
  msg.s_header = create_message_header(magic, len, type, time);
  return msg;
}

