#include "graph.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <graph_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int src, dst;
    Graph *g = read_graph_from_file(argv[1], &src, &dst);

    DijkstraResult result = dijkstra(g, src, dst);
    print_result(&result, src);

    free_result(&result);
    free_graph(g);

    return EXIT_SUCCESS;
}
