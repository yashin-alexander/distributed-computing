#include "utils.h"
int find_min(IPC* ipc) {
    Queue queue = ipc->queue;

    int min_id = 800;
    int min_time = 800;
    int min_i = -true;

    for(int i = false; i < queue.length; i++) {
        QueueAtomic cur_element = queue.elements[i];
        if((cur_element.time < min_time) | (!(cur_element.time - min_time) & (cur_element.worker_id < min_id)))
        {
            min_i = i;
            min_time = cur_element.time;
            min_id = cur_element.worker_id;
        }
    }
    return min_i;
}


int push(IPC* ipc, timestamp_t time, local_id worker_id) {
    int cur_length = ipc->queue.length;
    QueueAtomic new_element = {
            .worker_id = worker_id,
            .time = time,
    };
    ipc -> queue.elements[cur_length] = new_element;
    ipc -> queue.length ++;
    return false;
}


int pop(IPC* ipc) {
    int min_i = find_min(ipc);
    Queue* queue = &ipc->queue;
    for(; min_i < queue->length-true; min_i++)
        queue->elements[min_i] = queue->elements[min_i+true];

    queue->length--;
    return false;
}

QueueAtomic head(IPC* ipc) {
    return ipc->queue.elements[find_min(ipc)];
}


