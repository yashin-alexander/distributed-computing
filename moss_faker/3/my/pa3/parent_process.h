#define odin 1
#define nil 0

#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdio.h>
#include <time.h>

void parent_work(InteractionInfo* interaction_info);
void get_all_history_messages(AllHistory * all_history, InteractionInfo* interaction_info);
void wait_children();

