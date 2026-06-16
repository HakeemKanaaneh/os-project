#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Graph construction ── */

Graph *create_graph(int num_nodes, int num_edges) {
    Graph *g = malloc(sizeof(Graph));
    if (!g) { fprintf(stderr, "Error: malloc failed\n"); exit(EXIT_FAILURE); }
    g->num_nodes = num_nodes; g->num_edges = num_edges;
    g->adj = calloc(num_nodes, sizeof(AdjNode *));
    if (!g->adj) { fprintf(stderr, "Error: malloc failed\n"); free(g); exit(EXIT_FAILURE); }
    return g;
}

void add_edge(Graph *g, int src, int dst, int weight) {
    AdjNode *node = malloc(sizeof(AdjNode));
    if (!node) { fprintf(stderr, "Error: malloc failed\n"); exit(EXIT_FAILURE); }
    node->dest = dst; node->weight = weight;
    node->next = g->adj[src]; g->adj[src] = node;
}

void free_graph(Graph *g) {
    if (!g) return;
    for (int i = 0; i < g->num_nodes; i++) {
        AdjNode *cur = g->adj[i];
        while (cur) { AdjNode *t = cur; cur = cur->next; free(t); }
    }
    free(g->adj); free(g);
}

/* ── File reading ── */

SimData *read_sim_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) { fprintf(stderr, "Error: cannot open '%s'\n", filename); exit(EXIT_FAILURE); }

    SimData *data = calloc(1, sizeof(SimData));
    char line[256];
    int N = -1, M = -1;

    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, "%d %d", &N, &M) == 2) break;
    }
    if (N < 0 || M < 0) { fprintf(stderr, "Error: invalid header\n"); exit(EXIT_FAILURE); }

    data->graph = create_graph(N, M);
    int edges_read = 0;
    while (edges_read < M && fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        int s, d, w;
        if (sscanf(line, "%d %d %d", &s, &d, &w) != 3) {
            fprintf(stderr, "Error: invalid edge\n"); exit(EXIT_FAILURE);
        }
        if (s < 0 || s >= N || d < 0 || d >= N) {
            fprintf(stderr, "Error: node out of range\n"); exit(EXIT_FAILURE);
        }
        if (w < 0) { fprintf(stderr, "Error: negative weight\n"); exit(EXIT_FAILURE); }
        add_edge(data->graph, s, d, w);
        edges_read++;
    }

    int K = 0;
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, "%d", &K) == 1) break;
    }
    if (K <= 0 || K > MAX_TRAVELERS) {
        fprintf(stderr, "Error: invalid traveler count\n"); exit(EXIT_FAILURE);
    }
    data->num_travelers = K;

    int tr = 0;
    while (tr < K && fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        int s, d;
        if (sscanf(line, "%d %d", &s, &d) != 2) {
            fprintf(stderr, "Error: invalid traveler\n"); exit(EXIT_FAILURE);
        }
        data->travelers[tr].src = s;
        data->travelers[tr].dst = d;
        data->travelers[tr].pipe_fd = -1;
        data->travelers[tr].done = 0;
        tr++;
    }
    fclose(f);
    return data;
}

void free_sim_data(SimData *data) {
    if (!data) return;
    free_graph(data->graph);
    free(data);
}

/* ── Min-heap for Dijkstra ── */

typedef struct { int node, dist; } HeapNode;
typedef struct { HeapNode *data; int size, capacity; } MinHeap;

static MinHeap *create_heap(int cap) {
    MinHeap *h = malloc(sizeof(MinHeap));
    h->data = malloc(sizeof(HeapNode)*cap); h->size=0; h->capacity=cap; return h;
}
static void free_heap(MinHeap *h) { free(h->data); free(h); }
static void swap_h(HeapNode *a, HeapNode *b) { HeapNode t=*a; *a=*b; *b=t; }
static void heap_push(MinHeap *h, int node, int dist) {
    if (h->size==h->capacity) { h->capacity*=2; h->data=realloc(h->data,sizeof(HeapNode)*h->capacity); }
    h->data[h->size].node=node; h->data[h->size].dist=dist;
    int i=h->size++;
    while(i>0){int p=(i-1)/2; if(h->data[p].dist>h->data[i].dist){swap_h(&h->data[p],&h->data[i]);i=p;}else break;}
}
static HeapNode heap_pop(MinHeap *h) {
    HeapNode top=h->data[0]; h->data[0]=h->data[--h->size];
    int i=0;
    while(1){
        int l=2*i+1,r=2*i+2,s=i;
        if(l<h->size&&h->data[l].dist<h->data[s].dist)s=l;
        if(r<h->size&&h->data[r].dist<h->data[s].dist)s=r;
        if(s==i)break;
        swap_h(&h->data[i],&h->data[s]);
        i=s;
    }
    return top;
}

/* ── Dijkstra ── */

DijkstraResult dijkstra(Graph *g, int src, int dst) {
    int N=g->num_nodes;
    int *dist=malloc(sizeof(int)*N), *prev=malloc(sizeof(int)*N);
    for(int i=0;i<N;i++){dist[i]=INF;prev[i]=-1;}
    dist[src]=0;
    MinHeap *heap=create_heap(N+1); heap_push(heap,src,0);
    while(heap->size>0){
        HeapNode cur=heap_pop(heap);
        int u=cur.node;
        if(cur.dist>dist[u]) continue;
        if(u==dst) break;
        for(AdjNode *e=g->adj[u];e;e=e->next){
            int v=e->dest,alt=dist[u]+e->weight;
            if(alt<dist[v]){dist[v]=alt;prev[v]=u;heap_push(heap,v,alt);}
        }
    }
    free_heap(heap);

    DijkstraResult result={NULL,0,0,0};
    if(src==dst){result.found=1;result.path_len=1;result.path=malloc(sizeof(int));result.path[0]=src;free(dist);free(prev);return result;}
    if(dist[dst]==INF){free(dist);free(prev);return result;}

    int len=0; for(int v=dst;v!=-1;v=prev[v])len++;
    result.path=malloc(sizeof(int)*len); result.path_len=len; result.found=1; result.total_weight=dist[dst];
    int idx=len-1; for(int v=dst;v!=-1;v=prev[v])result.path[idx--]=v;
    free(dist); free(prev); return result;
}

void free_result(DijkstraResult *r) { if(r&&r->path){free(r->path);r->path=NULL;} }

void print_result(const DijkstraResult *r, int src) {
    (void)src;
    if(!r->found){printf("No path found\n");return;}
    for(int i=0;i<r->path_len;i++){if(i>0)printf(" -> ");printf("%d",r->path[i]);}
    printf("\n%d\n",r->total_weight);
}
