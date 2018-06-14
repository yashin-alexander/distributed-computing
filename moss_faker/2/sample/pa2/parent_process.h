#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>

void parent_work(InteractionInfo* interaction_info);

void get_all_history_messages(AllHistory * all_history, InteractionInfo* interaction_info);

void wait_children();
