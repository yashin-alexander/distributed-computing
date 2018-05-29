#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "banking.h"
#include "pa2345.h"
#include "ipc.h"


typedef struct {
    int read;
    int write;
} pipe_ends;

typedef struct {
    int id;
    pid_t pid;
    pipe_ends *pipes;
    uint8_t connection_count;
    int balance;
} proc;

typedef struct{
    uint8_t proc_count;
    proc *nodes;
} ProcContainer;

static const char *const log_close_read_end = "PID = %d CLOSED READ-end (%d) pipe FOR procs %d -> %d\n";
static const char *const log_close_write_end = "PID = %d CLOSED READ-end (%d) pipe FOR procs %d -> %d\n";
static const char *const log_create_pipe = "CREATED pipe FOR procs %d -> %d (READ: %d WRITE: %d)\n";
static const char *const log_sent_balance_history = "%d: process %1d sent balance history\n";
static const char *const log_sent_started = "%d: process %1d sent started message\n";
static const char *const log_received_all_history = "%d: process %1d received all history\n";

//BalanceHistory bh;
FILE *p_events_log;
FILE *p_pipes_log;

void proc_work(ProcContainer container, proc * self);
void prepare_logs();
void start_work(proc * self);
void done_work(proc * self);
void wait_all_procs_status(proc * self, int status);
void useful_work(proc * self);
void exit_work(ProcContainer container, proc * self);
void close_pipes(ProcContainer container, proc * self);
//void write_balance_history();
void transfer_processing(Message msg, proc * self);
void wait_all_procs_history(proc * self, AllHistory *allHistory);
void wait_proc_status(proc * self, int dst, int status);
//void full_fill_history(int max_time, BalanceHistory *bh);


void transfer(void *parent_data, local_id src, local_id dst, balance_t amount) {
    Message msg;
    TransferOrder transferOrder;
    transferOrder.s_amount = amount;
    transferOrder.s_dst = dst;
    transferOrder.s_src = src;
    memcpy(msg.s_payload, &transferOrder, sizeof(transferOrder));
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_local_time = get_physical_time();
    msg.s_header.s_type = TRANSFER;
    msg.s_header.s_payload_len = sizeof(transferOrder);

    send(parent_data, src, &msg);
    wait_proc_status(parent_data, dst, ACK);
}

int receive(void *self, local_id from, Message *msg) {
    proc *proc_tmp = (proc *) self;

    int fd = proc_tmp->pipes[from].read;

    if (read(fd, &msg->s_header, sizeof(msg->s_header)) > 0) {
        if (read(fd, msg->s_payload, msg->s_header.s_payload_len) >= 0) {
            return 0;
        }
    }

    return 1;
}

ProcContainer create_distributed_system(int * balances, int process_cnt) {
    ProcContainer container;
    container.proc_count = process_cnt;
    container.nodes = (proc*) calloc(process_cnt, sizeof(proc));

    for (int i = 0; i < process_cnt; i++) {
        proc tmp_proc;
        tmp_proc.connection_count = process_cnt;
        tmp_proc.balance = balances[i - 1];
        tmp_proc.pipes = (pipe_ends *) malloc(sizeof(pipe_ends) * process_cnt);

        for (int j = 0; j < process_cnt; j++) {
            if (i == j)
                continue;
            int flags;
            int tmp_pipe[2];
            pipe(tmp_pipe);
            tmp_proc.pipes[j].read = tmp_pipe[0];
            tmp_proc.pipes[j].write = tmp_pipe[1];

            flags = fcntl(tmp_proc.pipes[j].read, F_GETFL, 0);
            fcntl(tmp_proc.pipes[j].read, F_SETFL, flags | O_NONBLOCK);
            flags = fcntl(tmp_proc.pipes[j].write, F_GETFL, 0);
            fcntl(tmp_proc.pipes[j].write, F_SETFL, flags | O_NONBLOCK);

            fprintf(p_pipes_log, log_create_pipe, i, j, tmp_proc.pipes[j].read, tmp_proc.pipes[j].write);
            fflush(p_pipes_log);
        }
        container.nodes[i] = tmp_proc;
    }
    return container;
}

int receive_any(void *self, Message *msg) {
    proc *proc_tmp = (proc *) self;
    int flag = 1;
    for (int i = 0; i < proc_tmp->connection_count; i++) {
        if (proc_tmp->id == i) continue;
        if (receive(proc_tmp, i, msg) == 0) {
            flag = 0;
            break;
        }
    }
    return flag;
}


int send(void *self, local_id dst, const Message *msg) {
    proc *proc_tmp = (proc *) self;
    printf("TRY TO SEND TO %d\n", proc_tmp->id);
    write(proc_tmp->pipes[dst].write, msg, msg->s_header.s_payload_len + sizeof(MessageHeader));
    return 0;
}

int send_multicast(void *self, const Message *msg) {
    proc *proc_tmp = (proc *) self;

    for (int i = 0; i < proc_tmp->connection_count; i++) {
        if (i == proc_tmp->id)
            continue;

        write(proc_tmp->pipes[i].write, msg, msg->s_header.s_payload_len + sizeof(MessageHeader));
    }

    return 0;
}

void wait_proc_status(proc * self, int dst, int status) {
    Message msg;
    do {
        receive(self, dst, &msg);
    } while (msg.s_header.s_type != status);
}

void prepare_logs() {
    p_events_log = fopen(events_log, "w+");
    p_pipes_log = fopen(pipes_log, "w+");
}

//void write_balance_history(proc * self) {
//    BalanceState bs;
//    int current_time = get_physical_time();
//
//    bs.s_balance = self->balance;
//    bs.s_time = current_time;
//    bs.s_balance_pending_in = 0;
//
//    bh.s_history[current_time] = bs;
//    bh.s_history_len = current_time + 1;
//}

void wait_all_procs_history(proc * self, AllHistory *allHistory) {
    Message msg;
    int max_time = 0;
    for (int i = 1; i < self->connection_count; i++) {
        BalanceHistory bh;

        do {
            receive(self, i, &msg);
            if (msg.s_header.s_type == BALANCE_HISTORY) {
                bh = *(BalanceHistory *) msg.s_payload;
                if (bh.s_id != i)
                    continue;
                break;
            }
        } while (1);

        if (max_time < bh.s_history_len)
            max_time = bh.s_history_len;
//        full_fill_history(max_time, &bh);
        allHistory->s_history[i - 1] = bh;
    }

}

//void full_fill_history(int max_time, BalanceHistory *bh) {
//    bh->s_history[0].s_balance = start_balances[bh->s_id];
//
//    for (int i = 1; i < max_time; i++) {
//        if (bh->s_history[i].s_balance == 0) {
//            bh->s_history[i].s_balance = bh->s_history[i - 1].s_balance;
//            bh->s_history[i].s_time = i;
//            bh->s_history[i].s_balance_pending_in = 0;
//            if(i==bh->s_history_len)
//                bh->s_history_len++;
//        }
//    }
//}

void wait_all_procs_status(proc * self, int status) {
    Message msg;
    for (int i = 1; i < self->connection_count; i++) {
        if (i == self->id) continue;
        do {
            receive(self, i, &msg);
        } while (msg.s_header.s_type != status);
    }
}

void transfer_processing(Message msg, proc * self) {
    TransferOrder *order = (TransferOrder *) msg.s_payload;
    int dst = order->s_dst;
    int src = order->s_src;
    int amount = order->s_amount;
    if (dst == self->id) {
        fprintf(p_events_log, log_transfer_in_fmt, get_physical_time(), dst, amount, src);
        fflush(p_events_log);
        self->balance += amount;
        msg.s_header.s_type = ACK;
        send(self, PARENT_ID, &msg);
    } else {
        fprintf(p_events_log, log_transfer_out_fmt, get_physical_time(), src, amount, dst);
        fflush(p_events_log);
        self->balance -= amount;
        send(self, dst, &msg);
    }
//    write_balance_history(self);
}

void useful_work(proc * self) {
    Message msg;
    do {
        int code = receive_any(self, &msg);
        if (code != 1 && msg.s_header.s_type == TRANSFER)
            transfer_processing(msg, self);
    } while (msg.s_header.s_type != STOP);
}


void close_pipes(ProcContainer container, proc * self) {
    for (int i = 0; i < container.proc_count; i++) {
        for (int j = 0; j < container.proc_count; j++) {
            int tmp_read = container.nodes[i].pipes[j].read;
            int tmp_write = container.nodes[i].pipes[j].write;
            if (i == j)
                continue;
            if (i == self->id) {
                close(container.nodes[i].pipes[j].read);
                fprintf(p_pipes_log, log_close_read_end, self->id, tmp_read, i, j);
            } else {
                if (j == self->id) {
                    close(container.nodes[i].pipes[j].write);
                    fprintf(p_pipes_log, log_close_write_end, self->id, tmp_write, i, j);
                } else {
                    if (j == i) {
                        continue;
                    }
                    close(container.nodes[i].pipes[j].read);
                    close(container.nodes[i].pipes[j].write);
                    fprintf(p_pipes_log, log_close_write_end, self->id, tmp_write, i, j);
                    fprintf(p_pipes_log, log_close_read_end, self->id, tmp_read, i, j);
                }
            }
        }
    }
}

void proc_work(ProcContainer container, proc * self) {
    close_pipes(container, self);
    start_work(self);
    useful_work(self);
    done_work(self);
    exit_work(container, self);
}

void start_work(proc * self) {
    Message msg;
    sprintf(msg.s_payload, log_started_fmt, get_physical_time(), self->id, getpid(), getppid(), self->balance);
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_local_time = get_physical_time();
    msg.s_header.s_type = STARTED;
    msg.s_header.s_payload_len = strlen(msg.s_payload);

//    printf("msg will be sent to %d\n", self.);
    send(self, PARENT_ID, &msg);
    fprintf(p_events_log, log_sent_started, get_physical_time(), self->id);
    fflush(p_events_log);
}

void done_work(proc * self) {
    fprintf(p_events_log, log_done_fmt, get_physical_time(), self->id, self->balance);
    fflush(p_events_log);

    Message msg;
    sprintf(msg.s_payload, log_done_fmt, get_physical_time(), self->id, self->balance);
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_local_time = get_physical_time();
    msg.s_header.s_type = DONE;
    msg.s_header.s_payload_len = strlen(msg.s_payload);

    send_multicast(self, &msg);
    wait_all_procs_status(self, DONE);
    fprintf(p_events_log, log_received_all_done_fmt, get_physical_time(), self->id);
    fflush(p_events_log);

//    memcpy(msg.s_payload, &bh, sizeof(bh));
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_local_time = get_physical_time();
    msg.s_header.s_type = BALANCE_HISTORY;
//    msg.s_header.s_payload_len = sizeof(bh);

    send(self, PARENT_ID, &msg);

    fprintf(p_events_log, log_sent_balance_history, get_physical_time(), self->id);
    fflush(p_events_log);
}

void exit_work(ProcContainer container, proc * self) {
    for (int i = 0; i < self->connection_count; i++) {
        int tmp_read = container.nodes[PARENT_ID].pipes[i].read;
        close(container.nodes[PARENT_ID].pipes[i].read);
        fprintf(p_pipes_log, log_close_read_end, self->id, tmp_read, get_physical_time(), i);
    }
    fclose(p_events_log);
    fclose(p_pipes_log);
}

int main(int argc, const char *argv[]) {
    if (argc < 3 || (strcmp(argv[1], "-p") != 0)) {
        printf("use key -p <N> to set the number of child processes\n");
        return 1;
    }
    if (argc != atoi((argv[2])) + 3) {
        printf("Enter valid number of balances\n");
        return 1;
    }

    int balance_cnt = atoi(argv[2]);
    int proc_cnt = balance_cnt + 1;
    int node_balances [balance_cnt];

    prepare_logs();
    for (int i = 0; i < balance_cnt; i++)
        node_balances[i] = atoi(argv[i + 2]);

    ProcContainer container = create_distributed_system(node_balances, proc_cnt);

    container.nodes[0].pid = getpid();
    container.nodes[0].id = PARENT_ID;
    pid_t pid = getppid();
    for (int i = 1; i < proc_cnt; i++) {
        if ((pid = fork()) == 0) {
            container.nodes[i].pid = getpid();
            container.nodes[i].id = i;
//            bh.s_id = i;
            printf("proc run %d\n", i);
            proc_work(container, &container.nodes[i]);
            break;
        }
    }
    if (pid != 0) {
        close_pipes(container, &container.nodes[PARENT_ID]);
        wait_all_procs_status(&container.nodes[PARENT_ID], STARTED);
        fprintf(p_events_log, log_received_all_started_fmt, get_physical_time(), PARENT_ID);
        fflush(p_events_log);

        bank_robbery(container.nodes, proc_cnt - 1);

        Message msg;
        msg.s_header.s_magic = MESSAGE_MAGIC;
        msg.s_header.s_local_time = get_physical_time();
        msg.s_header.s_type = STOP;
        msg.s_header.s_payload_len = strlen(msg.s_payload);


        send_multicast(&container.nodes[PARENT_ID], &msg);

        wait_all_procs_status(&container.nodes[PARENT_ID], DONE);
        fprintf(p_events_log, log_received_all_done_fmt, get_physical_time(), PARENT_ID);
        fflush(p_events_log);

        AllHistory allHistory;
        allHistory.s_history_len = proc_cnt - 1;
        wait_all_procs_history(&container.nodes[PARENT_ID], &allHistory);

        fprintf(p_events_log, log_received_all_history, get_physical_time(), PARENT_ID);
        fflush(p_events_log);

        int proc_cnt_copy = proc_cnt;

        while (--proc_cnt_copy)
            wait(NULL);
        print_history(&allHistory);

    }
    return 0;
}
