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

typedef struct{
    int in;
    int out;
} Connection;

typedef struct{
    local_id id;
    NodeLifecycle lifecycle;
    int connections_count;
    Connection * connections;
} Node;

typedef struct{
    int units_number;
    Node *nodes;
} NodesContainer;



int send(void * self, local_id dst, const Message * msg){
//    Node * node = (Node *) self;
//    int pipe = node->connections[dst].in;
//
//    logging_pipe_write(pipes_log_fd, dst, pipe);
//    int symbols_count = sizeof(MessageHeader) + (*msg).s_header.s_payload_len;
//    write(pipe, msg, symbols_count);
    Node *node = (Node*) self;
//    Connection *c = &(send_node->connections[dst]);

//    int write_descriptor = (*connection).wd;

    int wd_fd = node->connections[dst].in;

    int write_count = sizeof(MessageHeader) + (*msg).s_header.s_payload_len;
    write(wd_fd, msg, write_count);

    return 0;
}

int receive(void * self, local_id from, Message * msg){
    Node * node = (Node *) self;
    int pipe = node->connections[from].out;

    logging_pipe_read(pipes_log_fd, from, pipe);

    int symbols_count = sizeof(MessageHeader) + (*msg).s_header.s_payload_len;
    if (read(pipe, msg, symbols_count) < 0){
        return -1;
    }
    return 0;
}

int send_multicast(void * self, const Message * msg) {
    Node *node = (Node*) self;
    for (int i = 1; i < node->connections_count; i++) {
        if (i != node->id) {
            if (send(self, i, msg) != 0) {
                return -1;
            }
        }
    }
    return 0;
}

int receive_any(void *self, Message *msg){
    Node *node = (Node*) self;
//    char rr [32];

    while(1){
        for (int i = 1; i < node->connections_count; i++){
            if (i != node->id){
//                sprintf(rr, "receive mes by %d from %d\n", node->id, i);
//                write(STDOUT_FILENO, rr, 32);
                if (receive(self, i, msg) == 0){
                    return i;
                }
            }
        }
    }
}

void mainproc_lifecycle(void *self){}

Message create_message_body(){
    Message msg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_local_time = 0;

    return msg;
}

Message create_message(int id){
    Message msg = create_message_body();
    sprintf(msg.s_payload, log_started_fmt, id, getpid(), getppid());
    msg.s_header.s_type = STARTED;
    msg.s_header.s_payload_len = strlen(msg.s_payload);
    return msg;
}

void worker_pre_synchronize(Node self){
    Message msg = create_message(self.id);
    sprintf(msg.s_payload, log_started_fmt, self.id, getpid(), getppid());

    logging_start(events_log_fd, self.id);

    for (int i = 1; i < self.connections_count; i++) {
        send_multicast(&self, &msg);
    }
    for (int i = 1; i < self.connections_count; i++){
        receive_any(&self, &msg);
    }
    logging_received_all_started(events_log_fd ,self.id);
}

void worker_payload(Node self){

}

void worker_post_synchronize(Node self){
    Message msg = create_message(self.id);
    sprintf(msg.s_payload, log_done_fmt, self.id);

    logging_done(events_log_fd, self.id);

    for (int i = 1; i < self.connections_count; i++) {
        send_multicast(&self, &msg);
    }
    for (int i = 1; i < self.connections_count; i++){
        receive_any(&self, &msg);
    }
    logging_received_all_done(events_log_fd ,self.id);
}

void worker_lifecycle(void *self){
    Node *node = (Node*) self;

    worker_pre_synchronize(*node);
    worker_payload(*node);
    worker_post_synchronize(*node);
}

void initialize_pipes(NodesContainer container, int index){
    Node node = container.nodes[index];
    for (int i = 1; i < container.units_number; i++){
        if (i != index){
            int fd[2];
            pipe(fd);

            node.connections[i].out = fd[0];
            container.nodes[i].connections[index].in = fd[1];

            logging_pipes_open(pipes_log_fd, index, i, fd[0], fd[1]);
        }
    }
}

Node create_node(local_id id, NodeLifecycle lifecycle, int units_number){
    Node node;
    node.id = id;
    node.lifecycle = lifecycle;
    node.connections = (Connection *) calloc(units_number, sizeof(Connection));
    node.connections_count = units_number;

    return node;
}

NodesContainer initialize_container(int units_number){
    NodesContainer container;
    container.units_number = units_number;

    container.nodes = (Node *) calloc(units_number, sizeof(Node));
    Node node = create_node(PARENT_ID, mainproc_lifecycle, units_number);
    container.nodes[PARENT_ID] = node;

    for (int i = 1; i < units_number; i++)
        container.nodes[i] = create_node(i, worker_lifecycle, units_number);

    for (int i = 1; i < units_number; i++){
        initialize_pipes(container, i);
    }

    /*for (int i = 1; i < units_number; i++){
        printf("%d OUT pipes = ", i);
        for (int j = 1; j < units_number; j++)
            printf("%d ", container.nodes[i].connections[j].out);
        printf("\n");
    }

    for (int i = 1; i < units_number; i++){
        printf("%d IN pipes = ", i);
        for (int j = 1; j < units_number; j++)
            printf("%d ", container.nodes[i].connections[j].in);
        printf("\n");
    }*/
    return container;
}

void run_worker_node(Node node){
    pid_t pid = fork();
    if (pid < 0)
        return;
    else if (pid == 0){
        node.lifecycle(&node);
        _exit(0);
    }
}

void run_container(NodesContainer container){
    for (int i = 1; i < container.units_number; i++)
        run_worker_node(container.nodes[i]);
}

int main(int argc, char **argv){
    uint8_t units_number = strtol(argv[2], (char **)NULL, 10) + 1;
    create_log_files();

    NodesContainer container = initialize_container(units_number);
    run_container(container);

    exit(0);
}
