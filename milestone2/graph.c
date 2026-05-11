#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ──────────────────────────────────────────────
   Graph construction / destruction
   ────────────────────────────────────────────── */

Graph *create_graph(int num_nodes, int num_edges) {
    Graph *g = (Graph *)malloc(sizeof(Graph));
    if (!g) {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    g->num_nodes = num_nodes;
    g->num_edges = num_edges;
    g->adj = (AdjNode **)calloc(num_nodes, sizeof(AdjNode *));
    if (!g->adj) {
        fprintf(stderr, "Error: memory allocation failed\n");
        free(g);
        exit(EXIT_FAILURE);
    }
    return g;
}

void add_edge(Graph *g, int src, int dst, int weight) {
    AdjNode *node = (AdjNode *)malloc(sizeof(AdjNode));
    if (!node) {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    node->dest   = dst;
    node->weight = weight;
    node->next   = g->adj[src];
    g->adj[src]  = node;
}

void free_graph(Graph *g) {
    if (!g) return;
    for (int i = 0; i < g->num_nodes; i++) {
        AdjNode *cur = g->adj[i];
        while (cur) {
            AdjNode *tmp = cur;
            cur = cur->next;
            free(tmp);
        }
    }
    free(g->adj);
    free(g);
}

/* ──────────────────────────────────────────────
   File reading
   ────────────────────────────────────────────── */

Graph *read_graph_from_file(const char *filename, int *src_out, int *dst_out) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: cannot open file '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    int N, M;
    if (fscanf(f, "%d %d", &N, &M) != 2 || N <= 0 || M < 0) {
        fprintf(stderr, "Error: invalid graph dimensions\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    Graph *g = create_graph(N, M);

    for (int i = 0; i < M; i++) {
        int s, d, w;
        if (fscanf(f, "%d %d %d", &s, &d, &w) != 3) {
            fprintf(stderr, "Error: invalid edge format at edge %d\n", i + 1);
            free_graph(g);
            fclose(f);
            exit(EXIT_FAILURE);
        }
        /* Validate node indices */
        if (s < 0 || s >= N || d < 0 || d >= N) {
            fprintf(stderr, "Error: node index out of range in edge %d\n", i + 1);
            free_graph(g);
            fclose(f);
            exit(EXIT_FAILURE);
        }
        /* Reject negative weights */
        if (w < 0) {
            fprintf(stderr, "Error: negative weight is invalid input\n");
            free_graph(g);
            fclose(f);
            exit(EXIT_FAILURE);
        }
        add_edge(g, s, d, w);
    }

    /* Last line: src dst for Dijkstra */
    int src, dst;
    if (fscanf(f, "%d %d", &src, &dst) != 2) {
        fprintf(stderr, "Error: missing source/destination query\n");
        free_graph(g);
        fclose(f);
        exit(EXIT_FAILURE);
    }
    if (src < 0 || src >= N || dst < 0 || dst >= N) {
        fprintf(stderr, "Error: source or destination node out of range\n");
        free_graph(g);
        fclose(f);
        exit(EXIT_FAILURE);
    }

    *src_out = src;
    *dst_out = dst;

    fclose(f);
    return g;
}

/* ──────────────────────────────────────────────
   Min-heap (priority queue) for Dijkstra
   ────────────────────────────────────────────── */

typedef struct {
    int node;
    int dist;
} HeapNode;

typedef struct {
    HeapNode *data;
    int size;
    int capacity;
} MinHeap;

static MinHeap *create_heap(int capacity) {
    MinHeap *h = (MinHeap *)malloc(sizeof(MinHeap));
    h->data     = (HeapNode *)malloc(sizeof(HeapNode) * capacity);
    h->size     = 0;
    h->capacity = capacity;
    return h;
}

static void free_heap(MinHeap *h) {
    free(h->data);
    free(h);
}

static void swap_nodes(HeapNode *a, HeapNode *b) {
    HeapNode tmp = *a;
    *a = *b;
    *b = tmp;
}

static void heap_push(MinHeap *h, int node, int dist) {
    if (h->size == h->capacity) {
        h->capacity *= 2;
        h->data = (HeapNode *)realloc(h->data, sizeof(HeapNode) * h->capacity);
    }
    h->data[h->size].node = node;
    h->data[h->size].dist = dist;
    int i = h->size++;
    /* Bubble up */
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->data[parent].dist > h->data[i].dist) {
            swap_nodes(&h->data[parent], &h->data[i]);
            i = parent;
        } else {
            break;
        }
    }
}

static HeapNode heap_pop(MinHeap *h) {
    HeapNode top = h->data[0];
    h->data[0]   = h->data[--h->size];
    /* Bubble down */
    int i = 0;
    while (1) {
        int left  = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;
        if (left  < h->size && h->data[left].dist  < h->data[smallest].dist) smallest = left;
        if (right < h->size && h->data[right].dist < h->data[smallest].dist) smallest = right;
        if (smallest == i) break;
        swap_nodes(&h->data[i], &h->data[smallest]);
        i = smallest;
    }
    return top;
}

/* ──────────────────────────────────────────────
   Dijkstra
   ────────────────────────────────────────────── */

DijkstraResult dijkstra(Graph *g, int src, int dst) {
    int N = g->num_nodes;
    int *dist = (int *)malloc(sizeof(int) * N);
    int *prev = (int *)malloc(sizeof(int) * N);

    for (int i = 0; i < N; i++) {
        dist[i] = INF;
        prev[i] = -1;
    }
    dist[src] = 0;

    MinHeap *heap = create_heap(N + 1);
    heap_push(heap, src, 0);

    while (heap->size > 0) {
        HeapNode cur = heap_pop(heap);
        int u = cur.node;

        if (cur.dist > dist[u]) continue; /* stale entry */
        if (u == dst) break;              /* early exit */

        for (AdjNode *edge = g->adj[u]; edge; edge = edge->next) {
            int v   = edge->dest;
            int alt = dist[u] + edge->weight;
            if (alt < dist[v]) {
                dist[v] = alt;
                prev[v] = u;
                heap_push(heap, v, alt);
            }
        }
    }

    free_heap(heap);

    DijkstraResult result;
    result.found        = 0;
    result.path         = NULL;
    result.path_len     = 0;
    result.total_weight = 0;

    if (src == dst) {
        /* Same source and destination */
        result.found        = 1;
        result.total_weight = 0;
        result.path_len     = 1;
        result.path         = (int *)malloc(sizeof(int));
        result.path[0]      = src;
        free(dist);
        free(prev);
        return result;
    }

    if (dist[dst] == INF) {
        /* No path found */
        free(dist);
        free(prev);
        return result;
    }

    /* Reconstruct path */
    int len = 0;
    for (int v = dst; v != -1; v = prev[v]) len++;

    result.path     = (int *)malloc(sizeof(int) * len);
    result.path_len = len;
    result.found    = 1;
    result.total_weight = dist[dst];

    int idx = len - 1;
    for (int v = dst; v != -1; v = prev[v]) {
        result.path[idx--] = v;
    }

    free(dist);
    free(prev);
    return result;
}

void free_result(DijkstraResult *result) {
    if (result && result->path) {
        free(result->path);
        result->path = NULL;
    }
}

/* ──────────────────────────────────────────────
   Output
   ────────────────────────────────────────────── */

void print_result(const DijkstraResult *result, int src) {
    (void)src; /* unused — kept in signature for potential future use */
    if (!result->found) {
        printf("No path found\n");
        return;
    }
    /* Print path */
    for (int i = 0; i < result->path_len; i++) {
        if (i > 0) printf(" -> ");
        printf("%d", result->path[i]);
    }
    printf("\n%d\n", result->total_weight);
}
