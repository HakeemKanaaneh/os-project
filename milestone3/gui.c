#include "graph.h"
#include "gui.h"
#include <raylib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ──────────────────────────────────────────────
   Layout
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
   Draw arrow
   ────────────────────────────────────────────── */

static void draw_arrow(Vector2 from, Vector2 to, Color color) {
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f) return;
    float ux = dx / len, uy = dy / len;
    Vector2 start = { from.x + ux * NODE_RADIUS, from.y + uy * NODE_RADIUS };
    Vector2 end   = { to.x   - ux * NODE_RADIUS, to.y   - uy * NODE_RADIUS };
    DrawLineEx(start, end, 2.0f, color);
    float arrow_size = 12.0f;
    float angle = atan2f(uy, ux);
    Vector2 tip = end;
    Vector2 p1  = { tip.x + arrow_size * cosf(angle + PI * 0.85f),
                    tip.y + arrow_size * sinf(angle + PI * 0.85f) };
    Vector2 p2  = { tip.x + arrow_size * cosf(angle - PI * 0.85f),
                    tip.y + arrow_size * sinf(angle - PI * 0.85f) };
    DrawTriangle(tip, p1, p2, color);
}

/* ──────────────────────────────────────────────
   Path helpers
   ────────────────────────────────────────────── */

static int is_path_edge(int u, int v, const DijkstraResult *r) {
    if (!r || !r->found) return 0;
    for (int i = 0; i < r->path_len - 1; i++)
        if (r->path[i] == u && r->path[i+1] == v) return 1;
    return 0;
}

static int is_path_node(int u, const DijkstraResult *r) {
    if (!r || !r->found) return 0;
    for (int i = 0; i < r->path_len; i++)
        if (r->path[i] == u) return 1;
    return 0;
}

/* ──────────────────────────────────────────────
   Get weight of edge u->v
   ────────────────────────────────────────────── */

static int edge_weight(Graph *g, int u, int v) {
    for (AdjNode *e = g->adj[u]; e; e = e->next)
        if (e->dest == v) return e->weight;
    return 1;
}

/* ──────────────────────────────────────────────
   Draw the static graph
   ────────────────────────────────────────────── */

static void draw_graph(Graph *g, const DijkstraResult *result,
                       Vector2 *pos, int src, int dst,
                       int W, int H) {
    (void)W; /* used only for path text position via H */
    /* Path info */
    if (result->found) {
        char info[1024], path_str[768] = "";
        for (int i = 0; i < result->path_len; i++) {
            char part[16];
            if (i > 0) snprintf(part, sizeof(part), " -> %d", result->path[i]);
            else        snprintf(part, sizeof(part), "%d",     result->path[i]);
            strncat(path_str, part, sizeof(path_str) - strlen(path_str) - 1);
        }
        snprintf(info, sizeof(info), "Path: %s   |   Total weight: %d",
                 path_str, result->total_weight);
        DrawText(info, 10, H - 30, 16, PATH_COLOR);
    } else {
        DrawText("No path found", 10, H - 30, 16, RED);
    }

    /* Edges */
    for (int u = 0; u < g->num_nodes; u++) {
        for (AdjNode *e = g->adj[u]; e; e = e->next) {
            int v       = e->dest;
            int on_path = is_path_edge(u, v, result);
            draw_arrow(pos[u], pos[v], on_path ? PATH_COLOR : EDGE_COLOR);
            float mx = (pos[u].x + pos[v].x) / 2.0f;
            float my = (pos[u].y + pos[v].y) / 2.0f;
            char wlabel[16];
            snprintf(wlabel, sizeof(wlabel), "%d", e->weight);
            DrawText(wlabel, (int)mx + 4, (int)my - 10, 14,
                     on_path ? PATH_COLOR : WEIGHT_COLOR);
        }
    }

    /* Nodes */
    for (int i = 0; i < g->num_nodes; i++) {
        Color fill    = is_path_node(i, result) ? PATH_COLOR : NODE_COLOR;
        Color outline = (i == src || i == dst)  ? GOLD       : DARKGRAY;
        DrawCircleV(pos[i], NODE_RADIUS, fill);
        DrawCircleLinesV(pos[i], NODE_RADIUS, outline);
        char label[16];
        snprintf(label, sizeof(label), "%d", i);
        int tw = MeasureText(label, 18);
        DrawText(label, (int)pos[i].x - tw/2, (int)pos[i].y - 9, 18, WHITE);
    }

    /* Legend */
    DrawCircleV((Vector2){20, 50}, 8, NODE_COLOR);
    DrawText("Node", 34, 44, 14, TEXT_COLOR);
    DrawCircleV((Vector2){20, 74}, 8, PATH_COLOR);
    DrawText("On shortest path", 34, 68, 14, TEXT_COLOR);
    DrawCircleV((Vector2){20, 98}, 8, GOLD);
    DrawText("Source / Destination", 34, 92, 14, TEXT_COLOR);
    DrawCircleV((Vector2){20, 122}, 8, ENTITY_COLOR);
    DrawText("Traveller", 34, 116, 14, TEXT_COLOR);
}

/* ──────────────────────────────────────────────
   Draw play/stop button — returns 1 if clicked
   ────────────────────────────────────────────── */

static int draw_button(int playing, int W) {
    Rectangle btn = { (float)(W - 120), 10, 110, 36 };
    Color col     = playing ? BTN_STOP_COLOR : BTN_COLOR;
    const char *label = playing ? "STOP" : "PLAY";

    DrawRectangleRec(btn, col);
    DrawRectangleLinesEx(btn, 2, WHITE);
    int tw = MeasureText(label, 18);
    DrawText(label, (int)(btn.x + (btn.width - tw) / 2),
             (int)(btn.y + 9), 18, WHITE);

    return (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            CheckCollisionPointRec(GetMousePosition(), btn));
}

/* ──────────────────────────────────────────────
   Update animation state
   ────────────────────────────────────────────── */

static void update_anim(AnimContext *anim, const DijkstraResult *result,
                        Vector2 *pos, Graph *g) {
    if (!anim->playing) return;
    if (anim->state == ANIM_DONE || anim->state == ANIM_IDLE) return;

    float dt = GetFrameTime();
    anim->timer += dt;

    if (anim->state == ANIM_MOVING) {
        /* Interpolate position along current edge */
        int from_node = result->path[anim->path_idx];
        int to_node   = result->path[anim->path_idx + 1];
        float t = (float)anim->step / (float)anim->total_steps;

        anim->entity_pos.x = pos[from_node].x + t * (pos[to_node].x - pos[from_node].x);
        anim->entity_pos.y = pos[from_node].y + t * (pos[to_node].y - pos[from_node].y);

        if (anim->timer >= STEP_TIME) {
            anim->timer = 0.0f;
            anim->step++;
            if (anim->step > anim->total_steps) {
                /* Reached next node */
                anim->path_idx++;
                anim->entity_pos = pos[to_node];

                if (anim->path_idx >= result->path_len - 1) {
                    anim->state = ANIM_DONE;
                } else {
                    /* Check if intermediate node — wait 1 second */
                    anim->state = ANIM_WAITING;
                    anim->timer = 0.0f;
                }
            }
        }

    } else if (anim->state == ANIM_WAITING) {
        if (anim->timer >= NODE_WAIT) {
            anim->timer = 0.0f;
            /* Start moving on next edge */
            int u = result->path[anim->path_idx];
            int v = result->path[anim->path_idx + 1];
            int w = edge_weight(g, u, v);
            anim->total_steps = w;
            anim->step        = 0;
            anim->state       = ANIM_MOVING;
        }
    }
}

/* ──────────────────────────────────────────────
   Main GUI
   ────────────────────────────────────────────── */

void run_gui(Graph *g, const DijkstraResult *result, int src, int dst) {
    const int W = WINDOW_WIDTH;
    const int H = WINDOW_HEIGHT;

    Vector2 *pos = (Vector2 *)malloc(sizeof(Vector2) * g->num_nodes);
    compute_layout(g->num_nodes, pos, W, H);

    InitWindow(W, H, "OS Project - Graph Simulation");
    SetTargetFPS(60);

    if (!IsWindowReady()) {
        fprintf(stderr, "Error: raylib window failed to initialize\n");
        free(pos);
        return;
    }

    /* Init animation */
    AnimContext anim;
    anim.state      = ANIM_IDLE;
    anim.playing    = 0;
    anim.path_idx   = 0;
    anim.step       = 0;
    anim.timer      = 0.0f;
    anim.entity_pos = pos[src];

    /* If no path, start in done state so entity just sits at src */
    if (!result->found) anim.state = ANIM_DONE;

    while (!WindowShouldClose()) {

        /* ── Button click ── */
        /* Handle in BeginDrawing block below */

        BeginDrawing();
        ClearBackground(BG_COLOR);

        DrawText("Graph Simulation", 10, 10, 20, TEXT_COLOR);

        /* Draw static graph */
        draw_graph(g, result, pos, src, dst, W, H);

        /* Draw play/stop button */
        int clicked = draw_button(anim.playing, W);
        if (clicked) {
            if (anim.state == ANIM_DONE) {
                /* Restart */
                anim.state      = ANIM_IDLE;
                anim.path_idx   = 0;
                anim.step       = 0;
                anim.timer      = 0.0f;
                anim.entity_pos = pos[src];
                anim.playing    = 1;
            }
            anim.playing = !anim.playing;

            /* If just started for first time */
            if (anim.playing && anim.state == ANIM_IDLE && result->found) {
                if (result->path_len == 1) {
                    anim.state = ANIM_DONE;
                } else {
                    int u = result->path[0];
                    int v = result->path[1];
                    int w = edge_weight(g, u, v);
                    anim.total_steps = w;
                    anim.step        = 0;
                    anim.state       = ANIM_MOVING;
                }
            }
        }

        /* Update animation */
        update_anim(&anim, result, pos, g);

        /* Draw entity */
        DrawCircleV(anim.entity_pos, NODE_RADIUS * 0.65f, ENTITY_COLOR);
        DrawCircleLinesV(anim.entity_pos, NODE_RADIUS * 0.65f, WHITE);

        /* Draw "Arrived!" message */
        if (anim.state == ANIM_DONE && result->found) {
            const char *msg = "Arrived at destination!";
            int tw = MeasureText(msg, 28);
            DrawRectangle(W/2 - tw/2 - 16, H/2 - 30, tw + 32, 56,
                          CLITERAL(Color){0, 0, 0, 180});
            DrawText(msg, W/2 - tw/2, H/2 - 16, 28, GREEN);
        }

        EndDrawing();
    }

    free(pos);
    CloseWindow();
}