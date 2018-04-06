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


MessageHeader create_message_header(uint16_t payload_len, MessageType type, timestamp_t local_time) {
    MessageHeader header = {
            .s_magic= MESSAGE_MAGIC,
            .s_payload_len = payload_len,
            .s_type = type,
            .s_local_time  = local_time
    };

    return header;
}

Message create_message(char *payload, uint16_t payload_len, MessageType type, timestamp_t local_time) {
    Message msg;
    msg.s_header = create_message_header(payload_len, type, local_time);
    for (uint16_t i = 0; i < payload_len; i++) {
        msg.s_payload[i] = payload[i];
    }

    return msg;
}

void set_descriptor_non_block(int fd) {
    int flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void create_non_block_pipe(int *fd) {
    pipe(fd);
    set_descriptor_non_block(fd[0]);
    set_descriptor_non_block(fd[1]);
}

void establish_unidirectional_connecton(Node *send_node, Node *receive_node) {
    int fd[2];
    create_non_block_pipe(fd);

    receive_node->connections[send_node->id].rd = fd[0];
    send_node->connections[receive_node->id].wd = fd[1];
}

void establish_connection(Node *a, Node *b) {
    establish_unidirectional_connecton(a, b);
    establish_unidirectional_connecton(b, a);
}

void establish_all_connections(const DistributedSystem *ds) {
    for (uint8_t i = 0; i <  (*ds).node_count; i++) {
        for (uint8_t j = i + 1; j <  (*ds).node_count; j++) {
            establish_connection(&( (*ds).nodes[i]), &( (*ds).nodes[j]));
        }
    }
}

void close_connection(Connection *connection) {
    close((*connection).rd);
    close((*connection).wd);
}

void close_extra_connections(const DistributedSystem *ds, local_id node_id) {
    for (uint8_t i = 0; i <  (*ds).node_count; i++) {
        if (i != node_id) {
            for (uint8_t j = 0; j <  (*ds).node_count; j++) {
                if (i != j && j != node_id) {
                    close_connection(&( (*ds).nodes[i].connections[j]));
                    close_connection(&( (*ds).nodes[j].connections[i]));
                }
                else if (j == node_id) {
                    close_connection(&( (*ds).nodes[i].connections[j]));
                }
            }
        }
    }
}

int c_send(Connection *connection, const Message *msg) {
    int write_descriptor = (*connection).wd;

    int write_count = sizeof(MessageHeader) + (*msg).s_header.s_payload_len;
    write(write_descriptor, msg, write_count);

    return 0;
}

int c_receive(Connection *connection, Message *msg) {
    int read_descriptor = (*connection).rd;

    int read_count = sizeof(MessageHeader) + (*msg).s_header.s_payload_len;
    if (read(read_descriptor, msg, read_count) < 0) {
        return -1;
    }
    return 0;
}

void init_node(const DistributedSystem *ds, local_id node_id, node_job job) {
    Node *node = &( (*ds).nodes[node_id]);

    (*node).id = node_id;
    (*node).connection_count =  (*ds).node_count;
    (*node).connections = (Connection*) calloc((*node).connection_count, sizeof(Connection));

    (*node).job = job;
}

int run_node(const DistributedSystem *ds, local_id node_id) {
    pid_t pid = fork();
    if (pid < 0) {
        return -1;
    }
    else if (pid == 0) {
        close_extra_connections(ds, node_id);

        Node *node = &( (*ds).nodes[node_id]);
        (*node).job(node);

        _exit(0);
    }
    return pid;
}

int send(void * self, local_id dst, const Message * msg) {
    Node *send_node = (Node*) self;
    Connection *c = &(send_node->connections[dst]);

    log_pipe_write_event(send_node->id, (*c).wd);
    return c_send(c, msg);
}

int send_multicast(void * self, const Message * msg) {
    Node *send_node = (Node*) self;
    for (int i = 0; i < send_node->connection_count; i++) {
        if (i != send_node->id) {
            if (send(self, i, msg) != 0) {
                return -1;
            }
        }
    }
    return 0;
}

int receive(void * self, local_id from, Message * msg) {
    Node *receive_node = (Node*) self;
    Connection *c = &(receive_node->connections[from]);

    log_pipe_read_event(receive_node->id, (*c).rd);
    return c_receive(c, msg);
}

int receive_any(void * self, Message * msg) {
    Node *receive_node = (Node*) self;
    while(1) {
        for (int i = 0; i < receive_node->connection_count; i++) {
            if (i != receive_node->id) {
                if (receive(self, i, msg) == 0)
                    return i;
            }
        }
    }
}

void init_main_node(const DistributedSystem *ds, node_job main_node_job) {
    init_node(ds, PARENT_ID, main_node_job);
}

void init_all_child_nodes(const DistributedSystem *ds, node_job job) {
    for (int i = 1; i <  (*ds).node_count; i++) {
        init_node(ds, i, job);
    }
}

void init_distirbuted_system(DistributedSystem *ds, uint8_t node_count, node_job main_node_job, node_job n_job) {
    (*ds).node_count = node_count;
    (*ds).nodes = (Node*) calloc(node_count, sizeof(Node));

    init_main_node(ds, main_node_job);
    init_all_child_nodes(ds, n_job);
    establish_all_connections(ds);
}

DistributedSystem create_distributed_system(uint8_t node_count, node_job main_node_job, node_job n_job) {
    DistributedSystem ds;
    init_distirbuted_system(&ds, node_count, main_node_job, n_job);
    return ds;
}

void run_all_child_nodes(const DistributedSystem *ds) {
    for (uint8_t i = 1; i <  (*ds).node_count; i++) {
        run_node(ds, i);
    }
}

void run_main_node(const DistributedSystem *ds) {
    close_extra_connections(ds, PARENT_ID);
    Node *main_node = &( (*ds).nodes[PARENT_ID]);
    main_node->job(main_node);
}

void wait_for_all_child_nodes_to_terminate(const DistributedSystem *ds) {
    for (uint8_t i = 1; i <  (*ds).node_count; i++) {
        wait(NULL);
    }
}

void run_distributed_system(const DistributedSystem *ds) {
    run_all_child_nodes(ds);
    run_main_node(ds);
    wait_for_all_child_nodes_to_terminate(ds);
}

void child_job(void *self) {
    Node *node = (Node*) self;

    log_started_event((*node).id);
    Message msg = create_message("", 0, STARTED, 0);
    int len = sprintf(msg.s_payload, log_started_fmt, (*node).id, getpid(), getppid());
    msg.s_header.s_payload_len = len;
    send_multicast(self, &msg);

    for (uint8_t i = 2; i < (*node).connection_count; i++) {
        receive_any(self, &msg);
    }
    log_received_all_started_event((*node).id);

    log_done_event((*node).id);
    msg = create_message("", 0, DONE, 0);
    len = sprintf(msg.s_payload, log_done_fmt, (*node).id);
    msg.s_header.s_payload_len = len;
    send_multicast(self, &msg);

    for (uint8_t i = 2; i < (*node).connection_count; i++) {
        Message m;
        receive_any(self, &m);
    }
    log_received_all_done_event((*node).id);
}

void main_node_job(void *self) {}

int main(int argc, char *argv[]) {
    int node_count = atoi(argv[2]) + 1;

    DistributedSystem ds = create_distributed_system(node_count, main_node_job, child_job);
    get_events_log_descriptor();
    run_distributed_system(&ds);
    return 0;
}
