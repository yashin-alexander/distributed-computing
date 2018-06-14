#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "banking.h"
#include "ipc_common.h"

void transfer(void * parent_data, local_id src, local_id dst, balance_t amount) {
  InteractionInfo *interaction_info = (InteractionInfo*)parent_data;

  TransferOrder to = {
    .s_dst = dst,
    .s_src = src,
    .s_amount = amount
  };

  Message m = {
    .s_header = {
      .s_type = TRANSFER,
      .s_magic = MESSAGE_MAGIC,
      .s_payload_len = sizeof(TransferOrder),
      .s_local_time = get_physical_time()
    }
  };

  memcpy(m.s_payload, &to, sizeof(TransferOrder));
  send(interaction_info, src, &m);
  receive(interaction_info, dst, &m);
}
