#ifndef GRAPH_H
#define GRAPH_H

#include <sys/types.h>
#include <semaphore.h>
#include <time.h>

#define MAX_NODES     100
#define MAX_TRAVELERS  20
#define INF    1000000000

/* Scheduling algorithms */
#define SCHED_FCFS 0
#define SCHED_SJF  1

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
#define MSG_REQUEST  0   /* child requests entry to node    */
#define MSG_ENTERING 1   /* parent grants entry             */
#define MSG_LEAVING  2   /* child leaving node              */
#define MSG_DONE     3   /* child reached destination       */
#define MSG_GRANT    4   /* parent → child: you may enter   */

/* IPC message */
typedef struct {
    pid_t pid;
    int   traveler_idx;
    int   msg_type;
    int   current_node;
    int   next_node;       /* -1 = destination              */
    int   remaining_path;  /* for SJF: steps left in path   */
    long  arrival_time;    /* for FCFS: time of request (ms)*/
} IPCMessage;

/* Shared memory — one semaphore per node to protect node occupancy */
typedef struct {
    sem_t node_sem[MAX_NODES];
} SharedMem;

/* Single traveler */
typedef struct {
    int    src;
    int    dst;
    pid_t  pid;
    int    pipe_read;    /* parent reads child updates here  */
    int    grant_write;  /* parent writes grants here        */
    int    grant_read;   /* child reads grants here          */
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
    SharedMem  *shm;
    int         shm_fd;
    int         sched_algo;   /* SCHED_FCFS or SCHED_SJF */
} SimData;

/* Node wait queue entry */
typedef struct WaitEntry {
    int   traveler_idx;
    int   remaining_path;
    long  arrival_time;
    struct WaitEntry *next;
} WaitEntry;

/* Node wait queue */
typedef struct {
    WaitEntry *head;
    int        occupied;   /* 1 if a traveler is currently inside */
} NodeQueue;

/* Graph functions */
Graph          *create_graph(int num_nodes, int num_edges);
void            add_edge(Graph *g, int src, int dst, int weight);
void            free_graph(Graph *g);
SimData        *read_sim_file(const char *filename, int sched_algo);
void            free_sim_data(SimData *data);

/* Dijkstra */
DijkstraResult  dijkstra(Graph *g, int src, int dst);
void            free_result(DijkstraResult *result);
void            print_result(const DijkstraResult *result, int src);

/* Scheduler */
long            get_time_ms(void);

#endif /* GRAPH_H */
