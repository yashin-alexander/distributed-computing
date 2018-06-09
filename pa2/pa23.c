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
static const char *const log_sent_balance_history = "%d: process %1d sent balance history\n";
static const char *const log_received_all_history = "%d: process %1d received all history\n";
static const char *const log_close_write_end = "PID = %d CLOSED WRITE-end (%d) pipe FOR procs %d -> %d\n";
typedef struct {
    int write;
    int read;
} pipe_ends;
static const char *const log_close_read_end = "PID = %d CLOSED READ-end (%d) pipe FOR procs %d -> %d\n";
static const char *const log_sent_started = "%d: process %1d sent started message\n";
static const char *const log_create_pipe = "CREATED pipe FOR procs %d -> %d (READ: %d WRITE: %d)\n";
typedef struct {
    pipe_ends *pipes;
    pid_t pid;
    int id;
} worker;
BalanceHistory bh;
int *start_balances;
worker *procs;
int balance;
int workers_cnt;
FILE *p_events_log;
FILE *p_pipes_log;
void open_nonblocking_pipes(worker tmp_worker, int worker_index, int workers_cnt);
void wait_all_procs_history(int id, AllHistory *allHistory);
void initialize_balances(int balance_cnt , char *argv[]);
void full_fill_history(int max_time, BalanceHistory *bh);
worker create_process(int worker_index, int workers_cnt);
void wait_proc_status(int id, int dst, int status);
void wait_all_procs_status(int id, int status);
void transfer_processing(Message msg, int id);
void create_processes(int workers_cnt);
void write_balance_history();
void create_main_process();
void usefull_work(int id);
void close_pipes(int id);
void start_work(int id);
void proc_work(int id);
void done_work(int id);
void exit_work(int id);
void prepare_logs();
void mian_proc(int workers_cnt);
void transfer(void *parent_data, local_id src, local_id dst, balance_t amount) {
    Message msg;
    TransferOrder transferOrder;
    transferOrder.s_dst = dst;
    transferOrder.s_src = src;
    transferOrder.s_amount = amount;
    memcpy(msg.s_payload, &transferOrder, sizeof(transferOrder));
    msg.s_header.s_payload_len = sizeof(transferOrder);
    msg.s_header.s_local_time = get_physical_time();
    msg.s_header.s_type = TRANSFER;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    send(&procs[PARENT_ID], src, &msg);
    wait_proc_status(PARENT_ID, dst, ACK);
}
int receive(void *self, local_id from, Message *msg) {
    worker *proc_tmp = (worker *) self;
    int fd = procs[from].pipes[proc_tmp->id].read;
    if (read(fd, &msg->s_header, sizeof(msg->s_header)) > 0)
        if (read(fd, msg->s_payload, msg->s_header.s_payload_len) >= 0)
            return 0;
    return 1;
}
int receive_any(void *self, Message *msg) {
    worker *proc_tmp = (worker *) self;
    for (int i = 0; i < workers_cnt; i++) {
        if (proc_tmp->id == i)
            continue;
        else if (receive(proc_tmp, i, msg) == 0)
            return 0;
    }
    return 1;
}
int send(void *self, local_id dst, const Message *msg) {
    worker *proc_tmp = (worker *) self;
    int wd_fd = proc_tmp->pipes[dst].write;
    printf("msg %s is sent to %d by %d, write pipe = %d\n", msg->s_payload, dst, proc_tmp->id, wd_fd);
    int symbols_count = msg->s_header.s_payload_len + sizeof(MessageHeader);
    write(wd_fd, msg, symbols_count);
    return 0;
}
int send_multicast(void *self, const Message *msg) {
    worker *proc_tmp = (worker *) self;
    for (int i = 0; i < workers_cnt; i++) {
        if (i == proc_tmp->id)
            continue;
        send(self ,i, msg);
    }
    return 0;
}
void wait_proc_status(int id, int dst, int status) {
    Message msg;
    do {
        receive(&procs[id], dst, &msg);
    } while (msg.s_header.s_type != status);
}
void wait_all_procs_history(int id, AllHistory *allHistory) {
    Message msg;
    int max_time = 0;
    for (int i = 1; i < workers_cnt; i++) {
        BalanceHistory bh;
        do {
            receive(&procs[id], i, &msg);
            if (msg.s_header.s_type == BALANCE_HISTORY) {
                bh = *(BalanceHistory *) msg.s_payload;
                if (bh.s_id != i)
                    continue;
                break;
            }
        } while (1);
        if (max_time < bh.s_history_len)
            max_time = bh.s_history_len;
        full_fill_history(max_time, &bh);
        allHistory->s_history[i - 1] = bh;
    }
}
void full_fill_history(int max_time, BalanceHistory *bh) {
    bh->s_history[0].s_balance = start_balances[bh->s_id];
    for (int i = 1; i < max_time; i++) {
        if (bh->s_history[i].s_balance == 0) {
            bh->s_history[i].s_balance = bh->s_history[i - 1].s_balance;
            bh->s_history[i].s_time = i;
            bh->s_history[i].s_balance_pending_in = 0;
            if(i==bh->s_history_len)
                bh->s_history_len++;
        }
    }
}
void prepare_logs() {
    p_pipes_log = fopen(pipes_log, "w");
    p_events_log = fopen(events_log, "w");
}
void wait_all_procs_status(int id, int status) {
    Message msg;
    for (int i = 1; i < workers_cnt; i++) {
        if (i == id)
            continue;
        do {
            receive(&procs[id], i, &msg);
        } while (msg.s_header.s_type != status);
    }
}
void write_balance_history() {
    BalanceState bs;
    bs.s_balance = balance;
    int current_time = get_physical_time();
    bs.s_balance_pending_in = 0;
    bs.s_time = current_time;
    bh.s_history[current_time] = bs;
    bh.s_history_len = current_time + 1;
}
void transfer_processing(Message msg, int id) {
    TransferOrder *order = (TransferOrder *) msg.s_payload;
    int dst = order->s_dst;
    int src = order->s_src;
    int amount = order->s_amount;
    if (dst == id) {
        fprintf(p_events_log, log_transfer_in_fmt, get_physical_time(),dst,amount, src);
        fflush(p_events_log);
        balance += amount;
        msg.s_header.s_type = ACK;
        send(&procs[id], PARENT_ID, &msg);
    } else {
        fprintf(p_events_log, log_transfer_out_fmt, get_physical_time(), src, amount, dst);
        fflush(p_events_log);
        balance -= amount;
        send(&procs[id], dst, &msg);
    }
    write_balance_history();
}
void usefull_work(int id) {
    Message msg;
    do {
        int code = receive_any(&procs[id], &msg);
//        printf("msg received, code= %d\n", code);
        if (code != 1 && msg.s_header.s_type == TRANSFER)
            transfer_processing(msg, id);
    } while (msg.s_header.s_type != STOP);
}


void create_non_block_pipe(int *fd){
    pipe(fd);
    int flags = fcntl(fd[0], F_GETFL);
    fcntl(fd[0], F_SETFL, flags | O_NONBLOCK);

    flags = fcntl(fd[1], F_GETFL);
    fcntl(fd[1], F_SETFL, flags | O_NONBLOCK);
}

void establish_unidirectional_connecton(worker *send_worker, worker *receive_worker){
    int fd[2];
    create_non_block_pipe(fd);
    send_worker->pipes[receive_worker->id].write = fd[1];
    receive_worker->pipes[send_worker->id].read = fd[0];
//    printf("set up 2 pipes from=%d to=%d\n", send_worker->id, receive_worker->id);
    fprintf(p_pipes_log, log_create_pipe, send_worker->id, receive_worker->id, fd[1], fd[0]);
}

void establish_all_connections(int proc_count){
    for (int i = 0; i < proc_count; i++){
//        printf("create pipes for worker %d\n", i);
        for (int j = i + 1; j < proc_count; j++) {
            establish_unidirectional_connecton(&procs[i], &procs[j]);
            establish_unidirectional_connecton(&procs[j], &procs[i]);
        }
//        printf("created pipes for worker %d\n", i);
    }
}

void create_processes(int workers_cnt) {
    for (int worker_index = 0; worker_index < workers_cnt; worker_index++)
        procs[worker_index] = create_process(worker_index, workers_cnt);
    establish_all_connections(workers_cnt);
    for (int i = 0; i < workers_cnt; i++) {
        for (int j = 0; j < workers_cnt; j++){
            printf("from worker %d to %d -> read:%d, write:%d\n", i, j, );
        }
    }
//    for (int worker_index = 1; worker_index < workers_cnt; worker_index++)
//        close_pipes(worker_index);
//    printf("         1      2      3     4");
//
//    for (int worker_index = 1; worker_index < workers_cnt; worker_index++) {
//        printf("\nworker %d: ", worker_index);
//        for (int nei_index = 0; nei_index < workers_cnt; nei_index++) {
//            if (worker_index == nei_index)
//                continue;
//            printf("%d/%d ",
//                   procs[worker_index].pipes[nei_index].read,
//                   procs[worker_index].pipes[nei_index].write
//            );
//        }
//    }
}
worker create_process(int worker_index, int workers_cnt) {
    worker tmp_worker;
    tmp_worker.pipes = (pipe_ends *) malloc(sizeof(pipe_ends) * workers_cnt);
    tmp_worker.id = worker_index;
    return tmp_worker;
}


//void open_nonblocking_pipes(worker tmp_worker, int worker_index, int workers_cnt) {
//    for (int j = 0; j < workers_cnt; j++) {
//        int flags;
//        int tmp_pipe[2];
//        pipe(tmp_pipe);
//        if (worker_index == j)
//            continue;
//        tmp_worker.pipes[j].write = tmp_pipe[1];
//        tmp_worker.pipes[j].read = tmp_pipe[0];
//        flags = fcntl(tmp_worker.pipes[j].write, F_GETFL, 0);
//        fcntl(tmp_worker.pipes[j].write, F_SETFL, flags | O_NONBLOCK);
//        flags = fcntl(tmp_worker.pipes[j].read, F_GETFL, 0);
//        fcntl(tmp_worker.pipes[j].read, F_SETFL, flags | O_NONBLOCK);
//        fprintf(p_pipes_log, log_create_pipe, worker_index, j, tmp_worker.pipes[j].read, tmp_worker.pipes[j].write);
//    }
//    sleep(1);
//}
void close_pipes(int id) {
    for (int i = 0; i < workers_cnt; i++) {
        for (int j = 0; j < workers_cnt; j++) {
            if (i == j) continue;
            int tmp_write = procs[i].pipes[j].write;
            int tmp_read = procs[i].pipes[j].read;
            if (j == id) {
                close(procs[i].pipes[j].write);
                fprintf(p_pipes_log, log_close_write_end, id, tmp_write, i, j);
            } else if (i == id) {
                close(procs[i].pipes[j].read);
                fprintf(p_pipes_log, log_close_read_end, id, tmp_read, i, j);
            } else {
                fprintf(p_pipes_log, log_close_read_end, id, tmp_read, i, j);
                fprintf(p_pipes_log, log_close_write_end, id, tmp_write, i, j);
                close(procs[i].pipes[j].write);
                close(procs[i].pipes[j].read);
            }
        }
    }
//    for (int nei_index = 0; nei_index < workers_cnt; nei_index++) {
//        if (id == nei_index)
//            continue;
//        printf("worker index = %d, connect to %d to read: %d \n",
//               id, nei_index,
//               procs[id].pipes[nei_index].read
//        );
//    }
}
void proc_work(int id) {
//    printf("runna proc work\n");
//    close_pipes(id);
//    printf("pipes losed for proc %d\n", id);
    start_work(id);
//    printf("start work ended for proc %d\n", id);
    usefull_work(id);
//    printf("useful work ended for proc %d\n", id);
    done_work(id);
//    printf("work done for proc %d\n", id);
    exit_work(id);
//    printf("work exited for proc %d\n", id);
}
void done_work(int id) {
    fprintf(p_events_log, log_done_fmt, get_physical_time(), id, balance);
    fflush(p_events_log);
    Message msg;
    sprintf(msg.s_payload, log_done_fmt, get_physical_time(), id, balance);
    msg.s_header.s_local_time = get_physical_time();
    msg.s_header.s_type = DONE;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = strlen(msg.s_payload);
    send_multicast(&procs[id], &msg);
    wait_all_procs_status(id, DONE);
    fprintf(p_events_log, log_received_all_done_fmt, get_physical_time(), id);
    memcpy(msg.s_payload, &bh, sizeof(bh));
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_local_time = get_physical_time();
    msg.s_header.s_type = BALANCE_HISTORY;
    msg.s_header.s_payload_len = sizeof(bh);
    send(&procs[id], PARENT_ID, &msg);
    fprintf(p_events_log, log_sent_balance_history, get_physical_time(), id);
    fflush(p_events_log);
}
void start_work(int id) {
    Message msg;
    sprintf(msg.s_payload, log_started_fmt, get_physical_time(), id, getpid(), getppid(), balance);
    msg.s_header.s_type = STARTED;
    msg.s_header.s_local_time = get_physical_time();
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = strlen(msg.s_payload);
    send(&procs[id], PARENT_ID, &msg);
    fprintf(p_events_log, log_sent_started, get_physical_time(), id);
    fflush(p_events_log);
}
void exit_work(int id) {
    for (int i = 0; i < workers_cnt; i++) {
        int tmp_read = procs[0].pipes[i].read;
        close(procs[0].pipes[i].read);
        fprintf(p_pipes_log, log_close_read_end, id, tmp_read, get_physical_time(), i);
    }
    fclose(p_pipes_log);
    fclose(p_events_log);
}
void initialize_balances(int balance_cnt, char *argv[]) {
    start_balances = (int *) malloc(sizeof(int) * balance_cnt);
    for (int i = 1; i < workers_cnt; i++) {
        start_balances[i] = atoi(argv[i + 2]);
    }
}
void create_main_process() {
    pid_t ppid = getppid();
    procs[0].id = PARENT_ID;
    procs[0].pid = getpid();
    for (int worker_index = 1; worker_index < workers_cnt; worker_index++) {
        if ((ppid = fork()) == 0) {
            bh.s_id = worker_index;
            balance = start_balances[worker_index];
            printf("balance = %d\n", balance);
            procs[worker_index].id = worker_index;
            procs[worker_index].pid = getpid();
            proc_work(worker_index);
            break;
        }
    }
}
void main_proc(int workers_cnt){
    pid_t ppid = getppid();
    if (ppid) {
        Message msg;
        AllHistory allHistory;
//        printf("ALL THE PROC IS STARTID\n");
        wait_all_procs_status(PARENT_ID, STARTED);
        fprintf(p_events_log, log_received_all_started_fmt, get_physical_time(), PARENT_ID);
        bank_robbery(procs, workers_cnt - 1);
        msg.s_header.s_magic = MESSAGE_MAGIC;
        msg.s_header.s_local_time = get_physical_time();
        msg.s_header.s_type = STOP;
        msg.s_header.s_payload_len = strlen(msg.s_payload);
        send_multicast(&procs[0], &msg);
        wait_all_procs_status(PARENT_ID, DONE);
        fprintf(p_events_log, log_received_all_done_fmt, get_physical_time(), PARENT_ID);
        allHistory.s_history_len = workers_cnt - 1;
        wait_all_procs_history(PARENT_ID, &allHistory);
        fprintf(p_events_log, log_received_all_history, get_physical_time(), PARENT_ID);
        fflush(p_events_log);
        int workers_cnt_copy = workers_cnt;
        while (--workers_cnt_copy) wait(NULL);
        print_history(&allHistory);
    }
}
int main(int argc, char *argv[]) {
    if (argc < 3 || strcmp(argv[1], "-p") || argc != (atoi((argv[2])) + 3)) {
        return 1;
    }
    int balance_cnt = atoi(argv[2]);
    workers_cnt = balance_cnt + 1;
    procs = (worker *) malloc(sizeof(worker) * workers_cnt);
    initialize_balances(balance_cnt, argv);
    prepare_logs();
    create_processes(workers_cnt);
    printf("processes crated!\n");
    create_main_process();
    main_proc(workers_cnt);

    return 0;
}
