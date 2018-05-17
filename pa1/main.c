#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

#include "ipc.h"
#include "common.h"
#include "pa1.h"

#include "logging.h"
#include "structures.h"



MessageHeader create_message_header(uint16_t payload_len, MessageType type, timestamp_t local_time){
    MessageHeader header;
    header.s_magic= MESSAGE_MAGIC;
    header.s_payload_len = payload_len;
    header.s_type = type;
    header.s_local_time = local_time;
    return header;
}

Message create_message(char *payload, uint16_t payload_len, MessageType type, timestamp_t local_time){
    Message msg = {
            .s_header = create_message_header(payload_len, type, local_time)
    };
    uint16_t i = 0;
    while (i < payload_len){
        msg.s_payload[i] = payload[i];
        i++;
    }
    return msg;
}


void create_non_block_pipe(int *fd){
    pipe(fd);
    int flags = fcntl(fd[0], F_GETFL);
    fcntl(fd[0], F_SETFL, flags | O_NONBLOCK);

    flags = fcntl(fd[1], F_GETFL);
    fcntl(fd[1], F_SETFL, flags | O_NONBLOCK);
}

void establish_unidirectional_connecton(Node *send_node, Node *receive_node){
    int fd[2];
    create_non_block_pipe(fd);
    send_node->connections[receive_node->id].in = fd[1];
    receive_node->connections[send_node->id].out = fd[0];
}

void establish_all_connections(const NodesContainer container){
    uint16_t i = 0;
    while(i <  container.node_count) {
        for (uint8_t j = i + 1; j < container.node_count; j++) {
            establish_unidirectional_connecton(&container.nodes[i], &container.nodes[j]);
            establish_unidirectional_connecton(&container.nodes[j], &container.nodes[i]);
        }
        i++;
    }
}

void close_connection(Connection connection){
    close(connection.out);
    close(connection.in);
}

void close_extra_connections(const NodesContainer *container, local_id node_id){
    for (int i = 0; i <  (*container).node_count; i++)
        if (i != node_id) {
            uint8_t j = 0;
            while (j < (*container).node_count) {
                if (i != j && j != node_id) {
                    close_connection(((*container).nodes[i].connections[j]));
                    close_connection(((*container).nodes[j].connections[i]));
                } else if (j == node_id)
                    close_connection(((*container).nodes[i].connections[j]));
                j++;
            }
        }
}


int c_receive(Connection *connection, Message *msg){
    int read_count = sizeof(MessageHeader) + (*msg).s_header.s_payload_len;
    int read_descriptor = (*connection).out;
    if (read(read_descriptor, msg, read_count) < 0)
        return -2;
    return 0;
}

void init_node(const NodesContainer container, local_id node_id, NodeLifecycle lifecycle){
    Node *node = &container.nodes[node_id];
    node->connection_count =  container.node_count;
    node->id = node_id;
    int memsize = node->connection_count * sizeof(Connection);
    node->connections = (Connection*) malloc(memsize);
    node->lifecycle = lifecycle;
}

int run_node(const NodesContainer *container, local_id node_id){
    pid_t pid = fork();
    if (pid < 0)
        return -2;
    if (!pid){
        close_extra_connections(container, node_id);
        Node *node = &( (*container).nodes[node_id]);
        (*node).lifecycle(node);
        _exit(0);
    }
    return pid;
}

int send(void * self, local_id dst, const Message * msg){
    log_pipe_write_event(((Node*) self)->id, ((Node*) self)->connections[dst].in);
    int wd_fd = ((Node*) self)->connections[dst].in;
    int write_count = sizeof(MessageHeader) + (*msg).s_header.s_payload_len;
    write(wd_fd, msg, write_count);

    return 0;
}

int send_multicast(void * self, const Message * msg){
    for (int i = 0; i < ((Node*) self)->connection_count; i++)
        if (i != ((Node *) self)->id)
            if (send(self, i, msg))
                return -2;
    return 0;
}

int receive(void * self, local_id from, Message * msg){
    Connection *c = &(((Node*) self)->connections[from]);
    int out_fd = (*c).out;
    log_pipe_read_event(((Node*) self)->id, out_fd);
    return c_receive(c, msg);
}

int receive_any(void * self, Message * msg){
    Node *receive_node = (Node*) self;
    do {
        for (int i = 0; i < receive_node->connection_count; i++) {
            if (i != receive_node->id)
                if (!receive(self, i, msg))
                    return i;
        }
        sleep(1);
    }
    while( 1 != 4);
}

void init_all_worker_nodes(const NodesContainer *container, NodeLifecycle lifecycle){
    int i = 1;
    do {
        init_node(*container, i, lifecycle);
        i++;
    }
    while (i <  (*container).node_count);
}


NodesContainer create_distributed_system(uint8_t node_count, NodeLifecycle main_NodeLifecycle, NodeLifecycle n_lifecycle){
    NodesContainer container;
    container.node_count = node_count;
    container.nodes = (Node*) calloc(node_count, sizeof(Node));

    init_node(container, PARENT_ID, main_NodeLifecycle);
    init_all_worker_nodes(&container, n_lifecycle);
    establish_all_connections(container);
    return container;
}

void run_all_worker_nodes(const NodesContainer *container){
    uint8_t i = 1;
    while (i <  (*container).node_count){
        run_node(container, i);
        i++;
    }
}

void run_main_node(const NodesContainer *container){
    close_extra_connections(container, PARENT_ID);
    Node *main_node = &( (*container).nodes[PARENT_ID]);
    main_node->lifecycle(main_node);
}

void wait_for_all_worker_nodes_to_terminate(const NodesContainer *container){
    uint8_t i = 1;
    while(1 != 4){
        if (i == (*container).node_count)
            return;
        wait(NULL);
        i ++;
    }
}

void run_distributed_system(const NodesContainer container){
    run_all_worker_nodes(&container);
    run_main_node(&container);
    wait_for_all_worker_nodes_to_terminate(&container);
}


void worker_pre_synchronize(Node node){
    log_started_event(node.id);
    Message msg = create_message("", 0, STARTED, 0);
    int len = sprintf(msg.s_payload, log_started_fmt, node.id, getpid(), getppid());
    msg.s_header.s_payload_len = len;
    send_multicast(&node, &msg);

    for (uint8_t i = 2; i < node.connection_count; i++)
        receive_any(&node, &msg);
    log_received_all_started_event(node.id);
}

void worker_payload(Node self){}

void worker_post_synchronize(Node node){
    log_done_event(node.id);
    Message msg = create_message("", 0, DONE, 0);
    int len = sprintf(msg.s_payload, log_done_fmt, node.id);
    msg.s_header.s_payload_len = len;
    send_multicast(&node, &msg);

    uint8_t i = 2;
    do {
        Message m;
        receive_any(&node, &m);
        i++;
    }
    while(i < node.connection_count);
    log_received_all_done_event(node.id);
}

void worker_lifecycle(void *self){
    Node *node = (Node*) self;

    worker_pre_synchronize(*node);
    worker_payload(*node);
    worker_post_synchronize(*node);
}

void main_node_lifecycle(void *self){}

int main(int argc, char *argv[]){
    int node_count = atoi(argv[2]);
    node_count++;
    create_log_files();
    NodesContainer container = create_distributed_system(node_count, main_node_lifecycle, worker_lifecycle);
    run_distributed_system(container);
    return 0;
}
