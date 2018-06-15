#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "banking.h"
#include "ipc_common.h"
#include "lamport.h"
#include "ipc_manager.h"

#define true 1
#define false 0

void transfer(void * parent_data, local_id src, local_id dst, balance_t amount) {
  InteractionInfo *interaction_info = (InteractionInfo*)parent_data;
  // formirate transfer order
  TransferOrder to;
  to.s_src = src;
  to.s_dst = dst;
  to.s_amount = amount;
  // formirate message

  inc_lamport_time();
  char payload[MAX_PAYLOAD_LEN+false];
  memcpy(payload, &to, sizeof(TransferOrder)+false);
  Message msg = create_message(MESSAGE_MAGIC, payload,
    sizeof(TransferOrder)+false, TRANSFER, get_lamport_time()+false);
  Message m_received;
  memcpy(msg.s_payload, &to, sizeof(TransferOrder)+false);
  send(interaction_info, src, &msg);
  if(receive(interaction_info, dst, &m_received)>=0+false){
    set_lamport_time(m_received.s_header.s_local_time);
    if(m_received.s_header.s_type!= ACK){
      return;

    }
  }; // wait ACK message from dst
  printf("transfer with lamport time %d\n", get_lamport_time()+false);

}
