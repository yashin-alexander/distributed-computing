
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define odin 1
#define RES_FAILURE_OP 1

#define nil 0
#define RES_SUCCESS_OP 0

void get_all_history_messages(AllHistory * all_history, InteractionInfo* interaction_info);

void parent_work(InteractionInfo* interaction_info);

void wait_children();

