#ifndef __CONNECTIONS__H
#define __CONNECTIONS__H

typedef void (*NodeLifecycle)(void*);

typedef struct{
    int out;
    int in;
} Connection;

typedef struct{
    local_id id;
    NodeLifecycle lifecycle;
    uint8_t connection_count;
    Connection * connections;
} Node;

typedef struct{
    uint8_t node_count;
    Node *nodes;
} NodesContainer;

#endif
