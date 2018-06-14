#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "banking.h"
#include "ipc_common.h"

void transfer(void * parent_data, local_id src, local_id dst, balance_t amount) {
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
}
