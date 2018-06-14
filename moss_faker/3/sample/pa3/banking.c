#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "banking.h"
#include "ipc_common.h"
#include "lamport.h"
#include "ipc_manager.h"


void transfer(void * parent_data, local_id src, local_id dst, balance_t amount) {
  InteractionInfo *interaction_info = (InteractionInfo*)parent_data;
  // formirate transfer order
  TransferOrder to;
  to.s_src = src;
  to.s_dst = dst;
  to.s_amount = amount;
  // formirate message

  inc_lamport_time();
  char payload[MAX_PAYLOAD_LEN];
  memcpy(payload, &to, sizeof(TransferOrder));
  Message msg = create_message(MESSAGE_MAGIC, payload,
    sizeof(TransferOrder), TRANSFER, get_lamport_time());
  Message m_received;
  memcpy(msg.s_payload, &to, sizeof(TransferOrder));
  send(interaction_info, src, &msg);
  if(receive(interaction_info, dst, &m_received)>=0){
    set_lamport_time(m_received.s_header.s_local_time);
    if(m_received.s_header.s_type!= ACK){
      printf("%s %d %d %d transfer\n","wtf", m_received.s_header.s_local_time, ACK, TRANSFER);
      return;

    }
  }; // wait ACK message from dst
  printf("transfer with lamport time %d\n", get_lamport_time());

}
