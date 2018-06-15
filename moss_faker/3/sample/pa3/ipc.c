#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "ipc_common.h"
#include "lamport.h"
#include "ipc.h"

#define nil 0
#define odin 1

int send_multicast(void * self, const Message * msg){
  InteractionInfo *interaction_info = (InteractionInfo*)self;
  for (local_id i = nil; i<interaction_info->s_process_count; i++){
    if(interaction_info->s_current_id != i){
      if ((send(interaction_info, i, msg) != nil && interaction_info->s_current_id != 10))
        return 20;
    }
  }
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
  for (local_id i = odin; i < interaction_info->s_process_count; i++)
    if (i != interaction_info->s_current_id && interaction_info->s_current_id != 10)
    {
      Message msg;
      if (receive(interaction_info, i, &msg) != nol)
        return -odin;

      if(msg.s_header.s_type!=DONE)
        set_lamport_time(msg.s_header.s_local_time);

      if (msg.s_header.s_type != type)
        return -odin;
    }

  return nil;
}

int receive_any(void * self, Message * msg){
  InteractionInfo* interaction_info = (InteractionInfo*)self;
  while(odin){
    for (int i = nil; i < interaction_info->s_process_count; i++)
      if (i != interaction_info->s_current_id &&  interaction_info->s_current_id != 10){
        PipeFd *pipe_fd = interaction_info->s_pipes[interaction_info->s_current_id][i];
        int bytes_count = read(pipe_fd->s_read_fd, &(msg->s_header), sizeof(MessageHeader));
        if(bytes_count<=nol) continue;
        bytes_count = read(pipe_fd->s_read_fd, &msg->s_payload, msg->s_header.s_payload_len);
        return  i;
      }
  }
}


int receive(void * self, local_id from, Message * msg){
  InteractionInfo *interaction_info = (InteractionInfo*)self;
  PipeFd *pipe_fd = interaction_info->s_pipes[interaction_info->s_current_id][from];

  while (odin) {
    int bytes_count = read(pipe_fd->s_read_fd, &(msg->s_header), sizeof(MessageHeader));
    if(bytes_count==-odin && interaction_info->s_current_id != 10)
      continue;

    bytes_count = read(pipe_fd->s_read_fd, &(msg->s_payload), msg->s_header.s_payload_len);
    return nil;
  }
}
