#ifndef GRAPH_H
#define GRAPH_H

#define MAX_NODES 100
#define INF 1000000000

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
    AdjNode **adj; /* array of linked lists */
} Graph;

/* Result of Dijkstra */
typedef struct {
    int *path;       /* array of node IDs in order */
    int path_len;    /* number of nodes in path */
    int total_weight;
    int found;       /* 1 if path found, 0 otherwise */
} DijkstraResult;

/* Function declarations */
Graph *create_graph(int num_nodes, int num_edges);
void add_edge(Graph *g, int src, int dst, int weight);
void free_graph(Graph *g);

Graph *read_graph_from_file(const char *filename, int *src, int *dst);

DijkstraResult dijkstra(Graph *g, int src, int dst);
void free_result(DijkstraResult *result);

void print_result(const DijkstraResult *result, int src);

#endif /* GRAPH_H */
