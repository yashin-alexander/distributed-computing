#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "ipc_common.h"
#include "ipc.h"

int send_multicast(void * self, const Message * msg){
  InteractionInfo *interaction_info = (InteractionInfo*)self;
  for (local_id i = 0; i<interaction_info->s_process_count; i++){
    if(interaction_info->s_current_id != i){
      if (send(interaction_info, i, msg) != 0)
        return 1;
    }
  }
	return 0;
}

int send(void * self, local_id dst, const Message * msg){
  InteractionInfo *interaction_info = (InteractionInfo*)self;
  int write_fd = interaction_info->s_pipes[interaction_info->s_current_id][dst]->s_write_fd;
  if(write(write_fd, msg, msg->s_header.s_payload_len + sizeof(MessageHeader)) <= 0){
    return 1;
  }

  return 0;
}

int receive_multicast(void * self, int16_t type){
  InteractionInfo* interaction_info = (InteractionInfo*)self;
  for (local_id i = 1; i < interaction_info->s_process_count; i++)
    if (i != interaction_info->s_current_id)
    {
      Message msg;
      if (receive(interaction_info, i, &msg) != 0)
        return -1;

      if (msg.s_header.s_type != type)
        return -1;
    }

  return 0;
}

int receive_any(void * self, Message * msg){
  InteractionInfo* interaction_info = (InteractionInfo*)self;
  while(1){
    for (local_id i = 0; i < interaction_info->s_process_count; i++)
      if (i != interaction_info->s_current_id){
        PipeFd *pipe_fd = interaction_info->s_pipes[interaction_info->s_current_id][i];
        int bytes_count = read(pipe_fd->s_read_fd, &(msg->s_header), sizeof(MessageHeader));
        if(bytes_count<=0)
          continue;
        bytes_count = read(pipe_fd->s_read_fd, &msg->s_payload, msg->s_header.s_payload_len);
        return (int) i;
      }
  }
}


int receive(void * self, local_id from, Message * msg){
  InteractionInfo *interaction_info = (InteractionInfo*)self;
  local_id id = interaction_info->s_current_id;
  PipeFd *pipe_fd = interaction_info->s_pipes[id][from];

  while (1) {
    int bytes_count = read(pipe_fd->s_read_fd, &(msg->s_header), sizeof(MessageHeader));
    if(bytes_count==-1)
      continue;

    bytes_count = read(pipe_fd->s_read_fd, &(msg->s_payload), msg->s_header.s_payload_len);
    return 0;
  }
}
