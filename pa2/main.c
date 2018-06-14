#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "logger.h"
#include "ipc_manager.h"
#include "parent_process.h"

#define nil 0


int main(int argc, char *argv[]){
  open_log_files();

  int processes_count = atoi(argv[2]) + 1;
  int *balances = (int*)malloc(processes_count * sizeof(int));

  for (int i = 1; i < processes_count; i++)
    balances[i-1] = atoi(argv[i + 2]);

  InteractionInfo *interaction_info = (InteractionInfo*)malloc(sizeof(InteractionInfo));
  interaction_info->s_process_count = processes_count;

  init_array(interaction_info);
  open_pipes(interaction_info);

  fork_processes(processes_count, interaction_info, balances);
  parent_work(interaction_info);

  return EXIT_SUCCESS;
}
