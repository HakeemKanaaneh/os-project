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

/* ── Child process ── */
static void child_run(int traveler_idx, int src, int dst,
                      Graph *g, int write_fd, SharedMem *shm) {
    pid_t pid = getpid();
    IPCMessage msg;
    msg.pid          = pid;
    msg.traveler_idx = traveler_idx;

    DijkstraResult route = dijkstra(g, src, dst);

    if (!route.found) {
        msg.msg_type     = MSG_DONE;
        msg.current_node = src;
        msg.next_node    = -1;
        write(write_fd, &msg, sizeof(msg));
        free_result(&route);
        close(write_fd);
        exit(EXIT_SUCCESS);
    }

    for (int i = 0; i < route.path_len; i++) {
        int node = route.path[i];
        int next = (i + 1 < route.path_len) ? route.path[i + 1] : -1;

        /* ── Acquire node semaphore (critical section entry) ── */
        /* Send WAITING message before blocking */
        msg.msg_type     = MSG_WAITING;
        msg.current_node = node;
        msg.next_node    = next;
        write(write_fd, &msg, sizeof(msg));

        sem_wait(&shm->node_sem[node]);  /* block until node is free */

        /* ── Inside critical section ── */
        msg.msg_type     = MSG_ENTERING;
        msg.current_node = node;
        msg.next_node    = next;
        write(write_fd, &msg, sizeof(msg));

        /* Wait 1 second inside node (only intermediate nodes) */
        if (i > 0 && i < route.path_len - 1) {
            usleep(1000000);
        }

        /* Release node semaphore */
        sem_post(&shm->node_sem[node]);

        /* If not last node: travel along edge */
        if (next != -1) {
            /* Find edge weight */
            int w = 1;
            for (AdjNode *e = g->adj[node]; e; e = e->next) {
                if (e->dest == next) { w = e->weight; break; }
            }
            /* Edge traversal: w * 300ms */
            usleep(w * 300000);
        }
    }

    /* Send DONE */
    msg.msg_type     = MSG_DONE;
    msg.current_node = route.path[route.path_len - 1];
    msg.next_node    = -1;
    write(write_fd, &msg, sizeof(msg));

    free_result(&route);
    close(write_fd);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    SimData *data = read_sim_file(argv[1]);
    int K = data->num_travelers;

    /* Create one pipe per traveler */
    int pipes[MAX_TRAVELERS][2];
    for (int i = 0; i < K; i++) {
        if (pipe(pipes[i]) < 0) { perror("pipe"); exit(EXIT_FAILURE); }
        fcntl(pipes[i][0], F_SETFL, O_NONBLOCK);
    }

    /* Fork one child per traveler */
    for (int i = 0; i < K; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(EXIT_FAILURE); }

        if (pid == 0) {
            for (int j = 0; j < K; j++) {
                close(pipes[j][0]);
                if (j != i) close(pipes[j][1]);
            }
            child_run(i, data->travelers[i].src, data->travelers[i].dst,
                      data->graph, pipes[i][1], data->shm);
        }

        data->travelers[i].pid     = pid;
        data->travelers[i].pipe_fd = pipes[i][0];
        close(pipes[i][1]);
    }

    /* Parent runs GUI */
    run_gui(data);

    /* Cleanup */
    for (int i = 0; i < K; i++) {
        kill(data->travelers[i].pid, SIGTERM);
        waitpid(data->travelers[i].pid, NULL, 0);
        close(data->travelers[i].pipe_fd);
    }

    free_sim_data(data);
    return EXIT_SUCCESS;
}
