#include "graph.h"
#include "gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>

/* ──────────────────────────────────────────────
   Child process
   ────────────────────────────────────────────── */
static void child_run(int traveler_idx, int src, int dst,
                      Graph *g, int write_fd, int grant_fd) {
    pid_t pid = getpid();
    IPCMessage msg;
    msg.pid          = pid;
    msg.traveler_idx = traveler_idx;

    DijkstraResult route = dijkstra(g, src, dst);

    if (!route.found) {
        msg.msg_type     = MSG_DONE;
        msg.current_node = src;
        msg.next_node    = -1;
        msg.remaining_path = 0;
        write(write_fd, &msg, sizeof(msg));
        free_result(&route);
        close(write_fd); close(grant_fd);
        exit(EXIT_SUCCESS);
    }

    for (int i = 0; i < route.path_len; i++) {
        int node = route.path[i];
        int next = (i + 1 < route.path_len) ? route.path[i + 1] : -1;
        int remaining = route.path_len - 1 - i; /* steps left after this node */

        /* Send REQUEST to parent */
        msg.msg_type      = MSG_REQUEST;
        msg.current_node  = node;
        msg.next_node     = next;
        msg.remaining_path = remaining;
        msg.arrival_time  = get_time_ms();
        write(write_fd, &msg, sizeof(msg));

        /* Wait for GRANT from parent (blocking read) */
        IPCMessage grant;
        read(grant_fd, &grant, sizeof(grant));

        /* ── Now inside node ── */
        msg.msg_type     = MSG_ENTERING;
        msg.current_node = node;
        msg.next_node    = next;
        write(write_fd, &msg, sizeof(msg));

        /* Wait 1 second at intermediate nodes */
        if (i > 0 && i < route.path_len - 1) {
            usleep(1000000);
        }

        /* Send LEAVING to parent */
        msg.msg_type     = MSG_LEAVING;
        msg.current_node = node;
        msg.next_node    = next;
        write(write_fd, &msg, sizeof(msg));

        /* Travel along edge */
        if (next != -1) {
            int w = 1;
            for (AdjNode *e = g->adj[node]; e; e = e->next)
                if (e->dest == next) { w = e->weight; break; }
            usleep(w * 300000);
        }
    }

    /* Send DONE */
    msg.msg_type     = MSG_DONE;
    msg.current_node = route.path[route.path_len - 1];
    msg.next_node    = -1;
    write(write_fd, &msg, sizeof(msg));

    free_result(&route);
    close(write_fd); close(grant_fd);
    exit(EXIT_SUCCESS);
}

/* ──────────────────────────────────────────────
   Node queue management
   ────────────────────────────────────────────── */

static NodeQueue node_queues[MAX_NODES];

/* ──────────────────────────────────────────────
   Main
   ────────────────────────────────────────────── */

int main(int argc, char *argv[]) {
    /* Parse args: ./sim -schd fcfs <file> or ./sim -schd sjf <file> */
    if (argc != 4 || strcmp(argv[1], "-schd") != 0) {
        fprintf(stderr, "Usage: %s -schd fcfs|sjf <file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int sched_algo;
    if (strcmp(argv[2], "fcfs") == 0)       sched_algo = SCHED_FCFS;
    else if (strcmp(argv[2], "sjf") == 0)   sched_algo = SCHED_SJF;
    else {
        fprintf(stderr, "Error: unknown scheduler '%s'. Use fcfs or sjf.\n", argv[2]);
        return EXIT_FAILURE;
    }

    SimData *data = read_sim_file(argv[3], sched_algo);
    int K = data->num_travelers;
    int N = data->graph->num_nodes;

    /* Init node queues */
    for (int i = 0; i < N; i++) {
        node_queues[i].head     = NULL;
        node_queues[i].occupied = 0;
    }

    /* Create pipes: child→parent (updates) and parent→child (grants) */
    int update_pipes[MAX_TRAVELERS][2];
    int grant_pipes[MAX_TRAVELERS][2];

    for (int i = 0; i < K; i++) {
        if (pipe(update_pipes[i]) < 0) { perror("pipe"); exit(EXIT_FAILURE); }
        if (pipe(grant_pipes[i])  < 0) { perror("pipe"); exit(EXIT_FAILURE); }
        fcntl(update_pipes[i][0], F_SETFL, O_NONBLOCK);
    }

    /* Fork children */
    for (int i = 0; i < K; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(EXIT_FAILURE); }

        if (pid == 0) {
            /* Child: close all parent-side fds */
            for (int j = 0; j < K; j++) {
                close(update_pipes[j][0]);
                if (j != i) close(update_pipes[j][1]);
                close(grant_pipes[j][1]);
                if (j != i) close(grant_pipes[j][0]);
            }
            child_run(i,
                      data->travelers[i].src,
                      data->travelers[i].dst,
                      data->graph,
                      update_pipes[i][1],
                      grant_pipes[i][0]);
        }

        data->travelers[i].pid         = pid;
        data->travelers[i].pipe_read   = update_pipes[i][0];
        data->travelers[i].grant_write = grant_pipes[i][1];
        data->travelers[i].grant_read  = grant_pipes[i][0];
        close(update_pipes[i][1]);
        close(grant_pipes[i][0]);
    }

    /* Parent runs GUI — passes node_queues pointer */
    data->sched_algo = sched_algo;
    run_gui(data, node_queues);

    /* Cleanup */
    for (int i = 0; i < K; i++) {
        kill(data->travelers[i].pid, SIGTERM);
        waitpid(data->travelers[i].pid, NULL, 0);
        close(data->travelers[i].pipe_read);
        close(data->travelers[i].grant_write);
    }

    /* Free node queues */
    for (int i = 0; i < N; i++) {
        WaitEntry *cur = node_queues[i].head;
        while (cur) { WaitEntry *t = cur; cur = cur->next; free(t); }
    }

    free_sim_data(data);
    return EXIT_SUCCESS;
}
