#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "banking.h"
#include "ipc_common.h"
#include "lamport.h"
#include "ipc_manager.h"

/*void transfer(void * parent_data, local_id src, local_id dst, balance_t amount) {
  InteractionInfo *interaction_info = (InteractionInfo*)parent_data;

  TransferOrder an_order = {
    .s_dst = dst,
    .s_src = src,
    .s_amount = amount
  };

  Message msg = {
    .s_header = {
      .s_type = TRANSFER,
      .s_magic = MESSAGE_MAGIC,
      .s_payload_len = sizeof(TransferOrder),
      .s_local_time = get_physical_time()
    }
  };

  memcpy(msg.s_payload, &an_order, sizeof(TransferOrder));
  if (send(interaction_info, src, &msg) != 0) {
    perror("Transfer(): Send failed");
    return;
  }

  if (receive(interaction_info, dst, &msg) != 0) {
    perror("Transfer(): Receive failed");
    return;
  }
}*/


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

