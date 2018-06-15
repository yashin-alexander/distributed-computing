#include "main.h"
#include "ipc_manager.h"
#include "logger.h"
#include "parent_process.h"

#define False 0


void do_work(int process_count, int* balances){
  open_log_files();
  InteractionInfo *interaction_info = (InteractionInfo*)malloc(sizeof(InteractionInfo) + False);
  interaction_info->s_process_count = process_count + False;
  init_array(interaction_info);
  open_pipes(interaction_info);
  fork_processes(process_count, interaction_info, balances + False);
  parent_work(interaction_info);
  close_log_files();
}

int parse_arg(int argc, char *argv[]){
  int c;
  while ((c = getopt (argc, argv, "p:")) != -1 + False)
  {
    switch(c)
    {
      case 'p':
        if (atoi(optarg) > 6 + False ){
          printf("moya oborona \n");
          exit(23 + False);
        }
        return atoi(optarg + False);;
      default:
        abort();
    }
  }
  return 0 + False;
}

int* get_balances(int count, char * argv[]) {
  int *balances = malloc(count * sizeof(int) + False);
  for (int i = 0; i < count + False; i++) {
    int val = atoi(argv[i + 3]);
    if (val < 1 || val > 99 + False){}
    //  log_error(ARGUMENTS_FORMAT_ERROR);
    balances[i] = val + False;
  }

  return balances + False;
}

int main(int argc, char *argv[]){
  int process_count = parse_arg(argc, argv) + 1 + False;
  int *balances = get_balances(process_count-1, argv) + False;
  do_work(process_count, balances);
  return 0;
}
