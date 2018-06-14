#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "banking.h"
#include "ipc_common.h"

void transfer(void * parent_data, local_id src, local_id dst, balance_t amount) {
  InteractionInfo *interaction_info = (InteractionInfo*)parent_data;

  Message m;
  TransferOrder to;
  Message m_received;
  m.s_header.s_type = TRANSFER;
  to.s_dst = dst;
  m.s_header.s_magic = MESSAGE_MAGIC;
  to.s_src = src;
  m.s_header.s_payload_len = sizeof(TransferOrder);
  to.s_amount = amount;
  m.s_header.s_local_time = get_physical_time();

  memcpy(m.s_payload, &to, sizeof(TransferOrder));
  send(interaction_info, src, &m);
  receive(interaction_info, dst, &m_received);
}
