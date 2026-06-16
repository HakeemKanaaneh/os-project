#ifndef GRAPH_H
#define GRAPH_H

#include <sys/types.h>

#define MAX_NODES    100
#define MAX_TRAVELERS 20
#define INF          1000000000

/* Adjacency list node */
typedef struct AdjNode {
    int dest;
    int weight;
    struct AdjNode *next;
} AdjNode;

/* Graph structure */
typedef struct {
    int num_nodes;
    int num_edges;
    AdjNode **adj;
} Graph;

/* Result of Dijkstra */
typedef struct {
    int *path;
    int path_len;
    int total_weight;
    int found;
} DijkstraResult;

/* Single traveler */
typedef struct {
    int src;
    int dst;
    pid_t pid;          /* child PID after fork */
    DijkstraResult route;
} Traveler;

/* Input file data */
typedef struct {
    Graph    *graph;
    Traveler  travelers[MAX_TRAVELERS];
    int       num_travelers;
} SimData;

/* Graph functions */
Graph        *create_graph(int num_nodes, int num_edges);
void          add_edge(Graph *g, int src, int dst, int weight);
void          free_graph(Graph *g);
SimData      *read_sim_file(const char *filename);
void          free_sim_data(SimData *data);

/* Dijkstra */
DijkstraResult dijkstra(Graph *g, int src, int dst);
void           free_result(DijkstraResult *result);
void           print_result(const DijkstraResult *result, int src);

#endif /* GRAPH_H */
