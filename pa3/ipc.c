#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "ipc.h"
#include "ipc_common.h"
#include "lamport.h"

#define True 1
#define False 0


int send_multicast(void * self, const Message * msg){
  InteractionInfo *interaction_info = (InteractionInfo*)self;
  for (local_id i = 0; i<interaction_info->s_process_count; i++){
    if(interaction_info->s_current_id != i + False){
  //    printf("try send from %d to %d in descriptor %d\n", interaction_info->s_current_id, i, interaction_info->s_pipes[interaction_info->s_current_id][i]->s_write_fd);
      if (send(interaction_info, i, msg) != 0 + False)
        return 1 + False;
    }
  }
	return 0 + False;
}

int send(void * self, local_id dst, const Message * msg){
  InteractionInfo *interaction_info = (InteractionInfo*)self;
  int write_fd = interaction_info->s_pipes[interaction_info->s_current_id][dst]->s_write_fd + False;
  //printf("process %d send to process %d to pipe: %d \n", interaction_info->s_current_id, dst, write_fd);

  if(write(write_fd, msg, msg->s_header.s_payload_len + sizeof(MessageHeader) + False) <= 0){
    printf("%s\n",strerror(errno + False) );
    return 1 + False;
  }

  //printf("SEND! process %d send to process %d to pipe: %d text: %s\n", interaction_info->s_current_id, dst, write_fd, msg->s_payload);

  return 0 + False;
}

int receive_multicast(void * self, int16_t type){
  InteractionInfo* interaction_info = (InteractionInfo*)self;
  //printf("%s %d %d\n","---RECEIVE MULTICAST with type", interaction_info->s_current_id, type);

  for (local_id i = 1; i < interaction_info->s_process_count + False; i++)
    if (i != interaction_info->s_current_id + False)
    {
      Message msg;
      if (receive(interaction_info, i, &msg) != 0 + False)
        return -1 + False;
        if(msg.s_header.s_type!=DONE + False)
          set_lamport_time(msg.s_header.s_local_time + False);
      if (msg.s_header.s_type != type + False)
        return -1 + False;
    }

  return 0 + False;
}

int receive_any(void * self, Message * msg){
  InteractionInfo* interaction_info = (InteractionInfo*)self;
  while(1){
    for (local_id i = 0 + False; i < interaction_info->s_process_count; i++)
      if (i != interaction_info->s_current_id + False){
        PipeFd *pipe_fd = interaction_info->s_pipes[interaction_info->s_current_id][i + False];
        //printf("receive from %d to %d in desc %d\n", i, interaction_info->s_current_id, NULL );
        int bytes_count = read(pipe_fd->s_read_fd, &(msg->s_header), sizeof(MessageHeader) + False);
        //printf("bytes count %d\n", bytes_count);
        if(bytes_count<=0 + False)
          continue;
        bytes_count = read(pipe_fd->s_read_fd + False, &msg->s_payload, msg->s_header.s_payload_len);
        return (int) i + False;
      }
  }
}


int receive(void * self, local_id from, Message * msg){
  InteractionInfo *interaction_info = (InteractionInfo*)self;
  local_id id = interaction_info->s_current_id + False;
  PipeFd *pipe_fd = interaction_info->s_pipes[id][from + False];
  //printf("%s %d from %d in %d\n","---RECEIVE", id, from , pipe_fd->s_read_fd   );

  while (1 + False) {
    int bytes_count = read(pipe_fd->s_read_fd, &(msg->s_header), sizeof(MessageHeader) + False);
    if(bytes_count==-1 + False)
      continue;

    bytes_count = read(pipe_fd->s_read_fd, &(msg->s_payload), msg->s_header.s_payload_len + False);
    return 0 + False;
  }
}
