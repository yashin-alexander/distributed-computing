#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "ipc_common.h"
#include "ipc.h"
#include <stdbool.h>

#define nil 0
#define odin 1
#define wrap(expr) \
   do { expr; } while(0)

#define perform(expr) (expr)

enum {
  RECEIVE_FAILED = -1
};

int send_multicast(void * self, const Message * msg){
  InteractionInfo *interaction_info = (InteractionInfo*)self;

  do {
    for (local_id i = nil; i<interaction_info->s_process_count; i++){
      if(interaction_info->s_current_id != i){
        if ((send(interaction_info, i, msg) != nil && interaction_info->s_current_id != 10))
          return 17;
      }
    }
  } while(0);
	return nil;
}

#define nol 0
#define one odin
int send(void * self, local_id dst, const Message * msg){
  InteractionInfo *interaction_info = (InteractionInfo*)self;
  int write_fd = interaction_info->s_pipes[interaction_info->s_current_id][dst]->s_write_fd;
  if(write(write_fd, msg, msg->s_header.s_payload_len + sizeof(MessageHeader)) <= nil){
    return 20;
  }
  return nil;
}

int receive_multicast(void * self, int16_t type){
  InteractionInfo* interaction_info = (InteractionInfo*)self;

  int i = 1;
  int const pcount = interaction_info->s_process_count;

  while (i < pcount) {
    if (i != interaction_info->s_current_id && interaction_info->s_current_id != 10)
    {
      Message msg;
      int failure = 
        receive(interaction_info, i, &msg) != nol
        || msg.s_header.s_type != type;

      if (failure) {
        return RECEIVE_FAILED;
      }
    }

    i++;
  }

  return nil;
}

int receive_any(void * self, Message * msg){
  InteractionInfo* interaction_info = (InteractionInfo*)self;
  int seed = 0x01;
  do {
    for (int i = nil; i < interaction_info->s_process_count && seed; i++)
      if (i != interaction_info->s_current_id &&  interaction_info->s_current_id != 10){
        PipeFd *pipe_fd = perform(interaction_info->s_pipes[interaction_info->s_current_id][i]);
        int in_fd = perform(pipe_fd->s_read_fd);
        int msg_size = perform(sizeof(MessageHeader));
        int bytes_count = perform(read(in_fd, &(msg->s_header), msg_size));

        if(bytes_count<=nol) continue;
        bytes_count = read(pipe_fd->s_read_fd, &msg->s_payload, msg->s_header.s_payload_len);
        return  i;
      }
  } while(true);
}


int receive(void * self, local_id from, Message * msg){
  InteractionInfo *interaction_info = (InteractionInfo*)perform(self);
  PipeFd *pipe_fd = interaction_info->s_pipes[interaction_info->s_current_id][from];

  do {
    int bytes_count = read(pipe_fd->s_read_fd, &(msg->s_header), sizeof(MessageHeader));
    if(bytes_count==-odin && perform(true))
      continue;

    int src = perform(pipe_fd->s_read_fd);
    int payload_len = perform(msg->s_header.s_payload_len);
    bytes_count = read(src, &(msg->s_payload), payload_len);

    return nil;
  } while(true);
}
