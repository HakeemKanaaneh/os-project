#include "graph.h"
#include "gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Read graph and travelers */
    SimData *data = read_sim_file(argv[1]);

    /* Parent computes Dijkstra for each traveler */
    for (int i = 0; i < data->num_travelers; i++) {
        data->travelers[i].route = dijkstra(data->graph,
                                            data->travelers[i].src,
                                            data->travelers[i].dst);
    }

    /* Fork one child per traveler */
    for (int i = 0; i < data->num_travelers; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            /* ── Child process ── */
            printf("[%d] started\n", getpid());
            fflush(stdout);
            /* Child sleeps until parent sends SIGTERM */
            while (1) pause();
            exit(EXIT_SUCCESS);
        }
        /* Parent records child PID */
        data->travelers[i].pid = pid;
    }

    /* Parent runs GUI */
    run_gui(data);

    /* After GUI closes, send SIGTERM to each child and wait */
    for (int i = 0; i < data->num_travelers; i++) {
        if (data->travelers[i].pid > 0) {
            kill(data->travelers[i].pid, SIGTERM);
        }
    }
    for (int i = 0; i < data->num_travelers; i++) {
        if (data->travelers[i].pid > 0) {
            waitpid(data->travelers[i].pid, NULL, 0);
        }
    }

    free_sim_data(data);
    return EXIT_SUCCESS;
}
