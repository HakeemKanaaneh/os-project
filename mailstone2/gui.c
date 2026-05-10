#include "graph.h"
#include "gui.h"
#include <raylib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ──────────────────────────────────────────────
   Layout: arrange nodes in a circle
   ────────────────────────────────────────────── */

void compute_layout(int num_nodes, Vector2 *positions, int width, int height) {
    float cx     = width  / 2.0f;
    float cy     = height / 2.0f;
    float radius = (float)(width < height ? width : height) * 0.35f;

    for (int i = 0; i < num_nodes; i++) {
        float angle = (2.0f * PI * i) / num_nodes - PI / 2.0f;
        positions[i].x = cx + radius * cosf(angle);
        positions[i].y = cy + radius * sinf(angle);
    }
}

/* ──────────────────────────────────────────────
   Draw an arrow from point A to point B
   ────────────────────────────────────────────── */

static void draw_arrow(Vector2 from, Vector2 to, Color color) {
    float NODE_R = NODE_RADIUS;

    /* Shorten line so it starts/ends at node border */
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f) return;

    float ux = dx / len;
    float uy = dy / len;

    Vector2 start = { from.x + ux * NODE_R, from.y + uy * NODE_R };
    Vector2 end   = { to.x   - ux * NODE_R, to.y   - uy * NODE_R };

    DrawLineEx(start, end, 2.0f, color);

    /* Arrowhead */
    float arrow_size = 12.0f;
    float angle      = atan2f(uy, ux);
    float a1 = angle + PI * 0.85f;
    float a2 = angle - PI * 0.85f;

    Vector2 tip = end;
    Vector2 p1  = { tip.x + arrow_size * cosf(a1), tip.y + arrow_size * sinf(a1) };
    Vector2 p2  = { tip.x + arrow_size * cosf(a2), tip.y + arrow_size * sinf(a2) };

    DrawTriangle(tip, p1, p2, color);
}

/* ──────────────────────────────────────────────
   Check if edge (u->v) is on the Dijkstra path
   ────────────────────────────────────────────── */

static int is_path_edge(int u, int v, const DijkstraResult *result) {
    if (!result || !result->found) return 0;
    for (int i = 0; i < result->path_len - 1; i++) {
        if (result->path[i] == u && result->path[i + 1] == v) return 1;
    }
    return 0;
}

static int is_path_node(int u, const DijkstraResult *result) {
    if (!result || !result->found) return 0;
    for (int i = 0; i < result->path_len; i++) {
        if (result->path[i] == u) return 1;
    }
    return 0;
}

/* ──────────────────────────────────────────────
   Main GUI function
   ────────────────────────────────────────────── */

void run_gui(Graph *g, const DijkstraResult *result, int src, int dst) {
    const int W = WINDOW_WIDTH;
    const int H = WINDOW_HEIGHT;

    /* Compute node positions */
    Vector2 *pos = (Vector2 *)malloc(sizeof(Vector2) * g->num_nodes);
    compute_layout(g->num_nodes, pos, W, H);

    InitWindow(W, H, "OS Project - Graph Visualizer");
    SetTargetFPS(60);

    if (!IsWindowReady()) {
        fprintf(stderr, "Error: raylib window failed to initialize\n");
        free(pos);
        return;
    }

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BG_COLOR);

        /* ── Title ── */
        DrawText("Graph Visualizer", 10, 10, 20, TEXT_COLOR);

        /* ── Draw path info ── */
        if (result->found) {
            char info[1024];
            /* Build path string */
            char path_str[768] = "";
            for (int i = 0; i < result->path_len; i++) {
                char part[16];
                if (i > 0) snprintf(part, sizeof(part), " -> %d", result->path[i]);
                else       snprintf(part, sizeof(part), "%d", result->path[i]);
                strncat(path_str, part, sizeof(path_str) - strlen(path_str) - 1);
            }
            snprintf(info, sizeof(info), "Path: %s   |   Total weight: %d",
                     path_str, result->total_weight);
            DrawText(info, 10, H - 30, 16, PATH_COLOR);
        } else {
            DrawText("No path found", 10, H - 30, 16, RED);
        }

        /* ── Draw edges ── */
        for (int u = 0; u < g->num_nodes; u++) {
            for (AdjNode *e = g->adj[u]; e; e = e->next) {
                int v        = e->dest;
                int on_path  = is_path_edge(u, v, result);
                Color color  = on_path ? PATH_COLOR : EDGE_COLOR;

                draw_arrow(pos[u], pos[v], color);

                /* Weight label at midpoint */
                float mx = (pos[u].x + pos[v].x) / 2.0f;
                float my = (pos[u].y + pos[v].y) / 2.0f;
                char wlabel[16];
                snprintf(wlabel, sizeof(wlabel), "%d", e->weight);
                DrawText(wlabel, (int)mx + 4, (int)my - 10, 14,
                         on_path ? PATH_COLOR : WEIGHT_COLOR);
            }
        }

        /* ── Draw nodes ── */
        for (int i = 0; i < g->num_nodes; i++) {
            int   on_path = is_path_node(i, result);
            Color fill    = on_path ? PATH_COLOR : NODE_COLOR;
            Color outline = (i == src || i == dst) ? GOLD : DARKGRAY;

            DrawCircleV(pos[i], NODE_RADIUS, fill);
            DrawCircleLinesV(pos[i], NODE_RADIUS, outline);

            /* Node ID label */
            char label[8];
            snprintf(label, sizeof(label), "%d", i);
            int tw = MeasureText(label, 18);
            DrawText(label,
                     (int)pos[i].x - tw / 2,
                     (int)pos[i].y - 9,
                     18, WHITE);
        }

        /* ── Legend ── */
        DrawCircleV((Vector2){20, 50}, 8, NODE_COLOR);
        DrawText("Node", 34, 44, 14, TEXT_COLOR);

        DrawCircleV((Vector2){20, 74}, 8, PATH_COLOR);
        DrawText("On shortest path", 34, 68, 14, TEXT_COLOR);

        DrawCircleV((Vector2){20, 98}, 8, GOLD);
        DrawText("Source / Destination", 34, 92, 14, TEXT_COLOR);

        EndDrawing();
    }

    free(pos);
    CloseWindow();
}
