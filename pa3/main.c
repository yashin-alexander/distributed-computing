#include "main.h"
#include "ipc_manager.h"
#include "logger.h"
#include "parent_process.h"


void do_work(int process_count, int* balances){
  open_log_files();
  InteractionInfo *interaction_info = (InteractionInfo*)malloc(sizeof(InteractionInfo));
  interaction_info->s_process_count = process_count;
  init_array(interaction_info);
  open_pipes(interaction_info);
  fork_processes(process_count, interaction_info, balances);
  parent_work(interaction_info);
  close_log_files();
}

int parse_arg(int argc, char *argv[]){
  int c;
  while ((c = getopt (argc, argv, "p:")) != -1)
  {
    switch(c)
    {
      case 'p':
        return atoi(optarg);;
      default:
        abort();
    }
  }
  return 0;
}

int* get_balances(int count, char * argv[]) {
  int *balances = malloc(count * sizeof(int));
  for (int i = 0; i < count; i++) {
    int val = atoi(argv[i + 3]);
    if (val < 1 || val > 99){}
    balances[i] = val;
  }

  return balances;
}

int main(int argc, char *argv[]){
  int process_count = parse_arg(argc, argv) + 1;
  int *balances = get_balances(process_count-1, argv);
  do_work(process_count, balances);
  return 0;
}
