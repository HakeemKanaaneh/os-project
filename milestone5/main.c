#include "graph.h"
#include "gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>
#include <string.h>

/* ── Child process: computes own Dijkstra, sends updates via pipe ── */
static void child_run(int traveler_idx, int src, int dst,
                      Graph *g, int write_fd) {
    /* Child computes its own path */
    DijkstraResult route = dijkstra(g, src, dst);

    IPCMessage msg;
    msg.pid          = getpid();
    msg.traveler_idx = traveler_idx;

    if (!route.found) {
        /* Send one message with current=src, next=-1 to signal no path */
	msg.msg_type     = 1;
        msg.current_node = src;
        msg.next_node    = -1;
        write(write_fd, &msg, sizeof(msg));
        free_result(&route);
        close(write_fd);
        exit(EXIT_SUCCESS);
    }

    /* Send position update for each node in path */
    for (int i = 0; i < route.path_len; i++) {
        msg.msg_type     = 0;   /* 0 = normal */
	msg.current_node = route.path[i];
        msg.next_node    = (i + 1 < route.path_len) ? route.path[i + 1] : -1;
        write(write_fd, &msg, sizeof(msg));

        /* Sleep to simulate travel time:
           - At each node except last: sleep for edge weight * 300ms
           - At intermediate nodes (not src, not dst): extra 1s wait */
        if (i < route.path_len - 1) {
            /* Find edge weight to next node */
            int w = 1;
            for (AdjNode *e = g->adj[route.path[i]]; e; e = e->next) {
                if (e->dest == route.path[i+1]) { w = e->weight; break; }
            }
            /* Edge traversal time: w * 300ms */
            usleep(w * 300000);

            /* Intermediate node wait: 1 second (not src, not dst) */
            if (i > 0 && i < route.path_len - 2) {
                usleep(1000000);
            }
        }
    }

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
        /* Set read end non-blocking so GUI doesn't freeze */
        fcntl(pipes[i][0], F_SETFL, O_NONBLOCK);
    }

    /* Fork one child per traveler */
    for (int i = 0; i < K; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(EXIT_FAILURE); }

        if (pid == 0) {
            /* Child: close all read ends and other write ends */
            for (int j = 0; j < K; j++) {
                close(pipes[j][0]);
                if (j != i) close(pipes[j][1]);
            }
            child_run(i, data->travelers[i].src, data->travelers[i].dst,
                      data->graph, pipes[i][1]);
            /* child_run calls exit() */
        }

        /* Parent: record pid and read end, close write end */
        data->travelers[i].pid     = pid;
        data->travelers[i].pipe_fd = pipes[i][0];
        close(pipes[i][1]);
    }

    /* Parent runs GUI — passes pipe fds so GUI can poll them */
    run_gui(data);

    /* Cleanup: kill remaining children and wait */
    for (int i = 0; i < K; i++) {
        kill(data->travelers[i].pid, SIGTERM);
        waitpid(data->travelers[i].pid, NULL, 0);
        close(data->travelers[i].pipe_fd);
    }

    free_sim_data(data);
    return EXIT_SUCCESS;
}
