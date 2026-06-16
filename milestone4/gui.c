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
    float cx = width / 2.0f, cy = height / 2.0f;
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
    float dx = to.x - from.x, dy = to.y - from.y;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 1.0f) return;
    float ux = dx/len, uy = dy/len;
    Vector2 start = {from.x + ux*NODE_RADIUS, from.y + uy*NODE_RADIUS};
    Vector2 end   = {to.x   - ux*NODE_RADIUS, to.y   - uy*NODE_RADIUS};
    DrawLineEx(start, end, 2.0f, color);
    float as = 12.0f, angle = atan2f(uy, ux);
    Vector2 tip = end;
    Vector2 p1 = {tip.x + as*cosf(angle+PI*0.85f), tip.y + as*sinf(angle+PI*0.85f)};
    Vector2 p2 = {tip.x + as*cosf(angle-PI*0.85f), tip.y + as*sinf(angle-PI*0.85f)};
    DrawTriangle(tip, p1, p2, color);
}

/* ──────────────────────────────────────────────
   Edge weight lookup
   ────────────────────────────────────────────── */

static int edge_weight(Graph *g, int u, int v) {
    for (AdjNode *e = g->adj[u]; e; e = e->next)
        if (e->dest == v) return e->weight;
    return 1;
}

/* ──────────────────────────────────────────────
   Draw static graph (no path highlight — multiple paths)
   ────────────────────────────────────────────── */

static void draw_graph(Graph *g, Vector2 *pos, int H) {
    /* Edges */
    for (int u = 0; u < g->num_nodes; u++) {
        for (AdjNode *e = g->adj[u]; e; e = e->next) {
            draw_arrow(pos[u], pos[e->dest], EDGE_COLOR);
            float mx = (pos[u].x + pos[e->dest].x) / 2.0f;
            float my = (pos[u].y + pos[e->dest].y) / 2.0f;
            char wlabel[16];
            snprintf(wlabel, sizeof(wlabel), "%d", e->weight);
            DrawText(wlabel, (int)mx+4, (int)my-10, 14, WEIGHT_COLOR);
        }
    }
    /* Nodes */
    for (int i = 0; i < g->num_nodes; i++) {
        DrawCircleV(pos[i], NODE_RADIUS, NODE_COLOR);
        DrawCircleLinesV(pos[i], NODE_RADIUS, DARKGRAY);
        char label[16];
        snprintf(label, sizeof(label), "%d", i);
        int tw = MeasureText(label, 18);
        DrawText(label, (int)pos[i].x - tw/2, (int)pos[i].y - 9, 18, WHITE);
    }
    (void)H;
}

/* ──────────────────────────────────────────────
   Draw play/stop button
   ────────────────────────────────────────────── */

static void draw_button(int playing, int W) {
    Rectangle btn = {(float)(W-120), 10, 110, 36};
    Color col = playing ? BTN_STOP_COLOR : BTN_COLOR;
    const char *label = playing ? "STOP" : "PLAY";
    DrawRectangleRec(btn, col);
    DrawRectangleLinesEx(btn, 2, WHITE);
    int tw = MeasureText(label, 18);
    DrawText(label, (int)(btn.x+(btn.width-tw)/2), (int)(btn.y+9), 18, WHITE);
}

/* ──────────────────────────────────────────────
   Update single traveler animation
   ────────────────────────────────────────────── */

static void update_traveler(TravelerAnim *anim, const DijkstraResult *route,
                            Vector2 *pos, Graph *g) {
    if (anim->state == ANIM_DONE || anim->state == ANIM_IDLE) return;

    float dt = GetFrameTime();
    anim->timer += dt;

    if (anim->state == ANIM_MOVING) {
        int from = route->path[anim->path_idx];
        int to   = route->path[anim->path_idx + 1];
        float t  = (float)anim->step / (float)anim->total_steps;
        anim->entity_pos.x = pos[from].x + t*(pos[to].x - pos[from].x);
        anim->entity_pos.y = pos[from].y + t*(pos[to].y - pos[from].y);

        if (anim->timer >= STEP_TIME) {
            anim->timer = 0.0f;
            anim->step++;
            if (anim->step > anim->total_steps) {
                anim->path_idx++;
                anim->entity_pos = pos[to];
                if (anim->path_idx >= route->path_len - 1) {
                    anim->state = ANIM_DONE;
                } else {
                    anim->state = ANIM_WAITING;
                    anim->timer = 0.0f;
                }
            }
        }
    } else if (anim->state == ANIM_WAITING) {
        if (anim->timer >= NODE_WAIT) {
            anim->timer = 0.0f;
            int u = route->path[anim->path_idx];
            int v = route->path[anim->path_idx + 1];
            anim->total_steps = edge_weight(g, u, v);
            anim->step = 0;
            anim->state = ANIM_MOVING;
        }
    }
}

/* ──────────────────────────────────────────────
   Start a traveler animation
   ────────────────────────────────────────────── */

static void start_traveler(TravelerAnim *anim, const DijkstraResult *route,
                           Vector2 *pos, Graph *g) {
    anim->path_idx = 0;
    anim->step     = 0;
    anim->timer    = 0.0f;
    anim->entity_pos = pos[route->path[0]];

    if (!route->found || route->path_len <= 1) {
        anim->state = ANIM_DONE;
    } else {
        int u = route->path[0], v = route->path[1];
        anim->total_steps = edge_weight(g, u, v);
        anim->state = ANIM_MOVING;
    }
}

/* ──────────────────────────────────────────────
   Main GUI
   ────────────────────────────────────────────── */

void run_gui(SimData *data) {
    const int W = WINDOW_WIDTH;
    const int H = WINDOW_HEIGHT;
    int N = data->graph->num_nodes;
    int K = data->num_travelers;

    Vector2 *pos = (Vector2 *)malloc(sizeof(Vector2) * N);
    compute_layout(N, pos, W, H);

    /* Init traveler animations */
    TravelerAnim *anims = (TravelerAnim *)calloc(K, sizeof(TravelerAnim));
    for (int i = 0; i < K; i++) {
        anims[i].state = ANIM_IDLE;
        if (data->travelers[i].route.found)
            anims[i].entity_pos = pos[data->travelers[i].src];
        else
            anims[i].state = ANIM_DONE;
    }

    int playing = 0;

    InitWindow(W, H, "OS Project - Multi-Traveler Simulation");
    SetTargetFPS(60);

    if (!IsWindowReady()) {
        fprintf(stderr, "Error: window failed to initialize\n");
        free(pos); free(anims); return;
    }

    while (!WindowShouldClose()) {

        /* ── Handle input BEFORE drawing ── */
        Rectangle btn_rect = {(float)(W-120), 10, 110, 36};
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            CheckCollisionPointRec(GetMousePosition(), btn_rect)) {
            playing = !playing;
            if (playing) {
                for (int i = 0; i < K; i++) {
                    if (anims[i].state == ANIM_IDLE) {
                        start_traveler(&anims[i], &data->travelers[i].route,
                                       pos, data->graph);
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(BG_COLOR);

        DrawText("Multi-Traveler Simulation", 10, 10, 20, TEXT_COLOR);

        /* Draw static graph */
        draw_graph(data->graph, pos, H);

        /* Draw play/stop button */
        draw_button(playing, W);

        /* Update and draw each traveler */
        int all_done = 1;
        for (int i = 0; i < K; i++) {
            if (playing) update_traveler(&anims[i], &data->travelers[i].route,
                                         pos, data->graph);
            if (anims[i].state != ANIM_DONE) all_done = 0;

            /* Draw entity */
            Color col = TRAVELER_COLORS[i % TRAVELER_COLOR_COUNT];
            DrawCircleV(anims[i].entity_pos, NODE_RADIUS * 0.6f, col);
            DrawCircleLinesV(anims[i].entity_pos, NODE_RADIUS * 0.6f, WHITE);

            /* Label traveler index */
            char tlabel[16];
            snprintf(tlabel, sizeof(tlabel), "T%d", i+1);
            int tw = MeasureText(tlabel, 12);
            DrawText(tlabel, (int)anims[i].entity_pos.x - tw/2,
                     (int)anims[i].entity_pos.y - 6, 12, WHITE);
        }

        /* Legend */
        int ly = 50;
        for (int i = 0; i < K; i++) {
            Color col = TRAVELER_COLORS[i % TRAVELER_COLOR_COUNT];
            DrawCircleV((Vector2){20, (float)ly}, 8, col);
            char leg[32];
            if (data->travelers[i].route.found)
                snprintf(leg, sizeof(leg), "T%d: %d->%d (pid %d)",
                         i+1, data->travelers[i].src,
                         data->travelers[i].dst,
                         (int)data->travelers[i].pid);
            else
                snprintf(leg, sizeof(leg), "T%d: no path", i+1);
            DrawText(leg, 34, ly-6, 13, TEXT_COLOR);
            ly += 22;
        }

        /* All done message */
        if (all_done && playing) {
            const char *msg = "All travelers arrived!";
            int tw = MeasureText(msg, 28);
            DrawRectangle(W/2-tw/2-16, H/2-30, tw+32, 56,
                          CLITERAL(Color){0,0,0,180});
            DrawText(msg, W/2-tw/2, H/2-16, 28, GREEN);
        }

        EndDrawing();
    }

    free(pos);
    free(anims);
    CloseWindow();
}