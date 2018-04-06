#ifndef __CONNECTIONS__H
#define __CONNECTIONS__H

typedef void (*node_job)(void*);

typedef struct {

    int rd;
    int wd;

} Connection;

typedef struct {

    local_id id;
    node_job job;

    uint8_t connection_count;
    Connection *connections;

} Node;

typedef struct {

    uint8_t node_count;
    Node *nodes;

} DistributedSystem;

#endif
