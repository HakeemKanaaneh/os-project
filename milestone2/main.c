#include "graph.h"
#include "gui.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <graph_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int src, dst;
    Graph *g = read_graph_from_file(argv[1], &src, &dst);

    /* Run Dijkstra and print result to terminal (same as milestone 1) */
    DijkstraResult result = dijkstra(g, src, dst);
    print_result(&result, src);

    /* Launch GUI */
    run_gui(g, &result, src, dst);

    free_result(&result);
    free_graph(g);

    return EXIT_SUCCESS;
}
