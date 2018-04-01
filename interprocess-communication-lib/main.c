#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "pa1.h"
#include "ipc.h"
#include "common.h"

#include "logging.h"

struct Node;
typedef void (*NodeLifecycle)(void *);

typedef struct {
    int in;
    int out;
} ConnectionChannels;

typedef struct {
    local_id id;
    NodeLifecycle lifecycle;
    int connections_count;
    ConnectionChannels *channels;
} Node;

typedef struct {
    int units_number;
    Node *nodes;
} NodesContainer;


Node create_node(local_id id, NodeLifecycle lifecycle, int units_number);
void run_container(NodesContainer container);
void run_worker_node(Node node);


void mainproc_lifecycle(void *self) {}


void worker_pre_synchronize(Node self) {
    logging_start(events_log_fd, self.id);


    for (int i = 0; i < self.connections_count; i++){
        if (i != self.id){
            int pipe = self.channels->in;
            write(pipe, "HELLO YOUBA\n", 12);
        }
    }
    for (int i = 0; i < self.connections_count; i++){
        if (i != self.id){
            char msg [11];
            int pipe = self.channels->out;
            read(pipe, msg, 11);
            printf("%s", msg);
        }
    }
}

void worker_payload(Node self) {

}

void worker_post_synchronize(Node self) {
    logging_done(events_log_fd, self.id);
}

void worker_lifecycle(void *self) {
    Node *node = (Node*) self;

    worker_pre_synchronize(*node);
    worker_payload(*node);
    worker_post_synchronize(*node);
}


NodesContainer initialize_container(int units_number) {
    NodesContainer container;
    container.units_number = units_number;

    container.nodes = (Node *) calloc(units_number, sizeof(Node));
    Node node = create_node(PARENT_ID, mainproc_lifecycle, units_number);
    container.nodes[PARENT_ID] = node;

    for (int i = 1; i < units_number; i++)
        container.nodes[i] = create_node(i, worker_lifecycle, units_number);

    return container;
}

Node create_node(local_id id, NodeLifecycle lifecycle, int units_number) {
    Node node;
    node.id = id;
    node.lifecycle = lifecycle;
    node.channels = (ConnectionChannels *) calloc(units_number, sizeof(ConnectionChannels));
    node.connections_count = units_number;

    return node;
}

void run_container(NodesContainer container) {
    for (int i = 1; i < container.units_number; i++)
        run_worker_node(container.nodes[i]);
}

void run_worker_node(Node node){
    pid_t pid = fork();
    if (pid < 0)
        return;
    else if (pid == 0) {
        node.lifecycle(&node);
        _exit(0);
    }
}

int main(int argc, char **argv) {
    uint8_t units_number = strtol(argv[2], (char **)NULL, 10) + 1;
    create_log_files();

    NodesContainer container = initialize_container(units_number);
    run_container(container);

    exit(0);
}
