#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdio.h>
#include <time.h>

#include "ipc_common.h"
#include "banking.h"
#include "common.h"
#include "pa2345.h"
#include "ipc.h"

void child_work(local_id id, InteractionInfo* interaction_info, balance_t start_balance);
void wait_other_start(InteractionInfo* interaction_info);
void payload(InteractionInfo* interaction_info);
void handle_stop_msg(InteractionInfo* interaction_info, balance_t balance);
int handle_done_msg(InteractionInfo* interaction_info, int done_count, int process_count, timestamp_t last_time,
   BalanceHistory* history, int isInStopState, int isHistoryRequired);
balance_t handle_transfer(InteractionInfo* interaction_info, Message* msg,
  BalanceHistory* history, BalanceState* state, balance_t balance, timestamp_t* last_time);
void send_history_message(InteractionInfo* interaction_info, BalanceHistory *history);
