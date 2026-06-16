#ifndef GRAPH_H
#define GRAPH_H

#include <sys/types.h>
#include <semaphore.h>

#define MAX_NODES     100
#define MAX_TRAVELERS  20
#define INF    1000000000

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
    int  path_len;
    int  total_weight;
    int  found;
} DijkstraResult;

/* ── IPC message types ── */
#define MSG_WAITING  0   /* traveler waiting outside node  */
#define MSG_ENTERING 1   /* traveler entering node         */
#define MSG_LEAVING  2   /* traveler leaving node          */
#define MSG_DONE     3   /* traveler reached destination   */

/* IPC message child → parent */
typedef struct {
    pid_t pid;
    int   traveler_idx;
    int   msg_type;      /* MSG_* above                   */
    int   current_node;
    int   next_node;     /* -1 = destination              */
} IPCMessage;

/* Shared memory layout — one semaphore per node */
typedef struct {
    sem_t node_sem[MAX_NODES];  /* binary semaphore per node */
} SharedMem;

/* Single traveler */
typedef struct {
    int    src;
    int    dst;
    pid_t  pid;
    int    pipe_fd;
    int    current_node;
    int    next_node;
    int    waiting;
    int    done;
} Traveler;

/* Input file data */
typedef struct {
    Graph      *graph;
    Traveler    travelers[MAX_TRAVELERS];
    int         num_travelers;
    SharedMem  *shm;       /* pointer to shared memory */
    int         shm_fd;    /* shm file descriptor      */
} SimData;

/* Graph functions */
Graph          *create_graph(int num_nodes, int num_edges);
void            add_edge(Graph *g, int src, int dst, int weight);
void            free_graph(Graph *g);
SimData        *read_sim_file(const char *filename);
void            free_sim_data(SimData *data);

/* Dijkstra */
DijkstraResult  dijkstra(Graph *g, int src, int dst);
void            free_result(DijkstraResult *result);
void            print_result(const DijkstraResult *result, int src);

#endif /* GRAPH_H */
