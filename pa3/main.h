#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>

void do_work(int process_count, int* balances);

int parse_arg(int argc, char *argv[]);

int* get_balances(int count, char * argv[]);
