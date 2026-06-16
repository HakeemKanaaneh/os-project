#include "graph.h"
#include "gui.h"
#include <raylib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ──────────────────────────────────────────────
   Layout
   ────────────────────────────────────────────── */

void compute_layout(int num_nodes, Vector2 *positions, int width, int height) {
    float cx = width/2.0f, cy = height/2.0f;
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
    float dx=to.x-from.x, dy=to.y-from.y;
    float len=sqrtf(dx*dx+dy*dy); if(len<1.0f)return;
    float ux=dx/len, uy=dy/len;
    Vector2 start={from.x+ux*NODE_RADIUS, from.y+uy*NODE_RADIUS};
    Vector2 end  ={to.x  -ux*NODE_RADIUS, to.y  -uy*NODE_RADIUS};
    DrawLineEx(start, end, 2.0f, color);
    float as=12.0f, angle=atan2f(uy,ux);
    Vector2 tip=end;
    Vector2 p1={tip.x+as*cosf(angle+PI*0.85f), tip.y+as*sinf(angle+PI*0.85f)};
    Vector2 p2={tip.x+as*cosf(angle-PI*0.85f), tip.y+as*sinf(angle-PI*0.85f)};
    DrawTriangle(tip, p1, p2, color);
}

/* ──────────────────────────────────────────────
   Draw static graph
   ────────────────────────────────────────────── */

static void draw_graph(Graph *g, Vector2 *pos) {
    for (int u = 0; u < g->num_nodes; u++) {
        for (AdjNode *e = g->adj[u]; e; e = e->next) {
            draw_arrow(pos[u], pos[e->dest], EDGE_COLOR);
            float mx=(pos[u].x+pos[e->dest].x)/2.0f;
            float my=(pos[u].y+pos[e->dest].y)/2.0f;
            char wlabel[16]; snprintf(wlabel,sizeof(wlabel),"%d",e->weight);
            DrawText(wlabel,(int)mx+4,(int)my-10,14,WEIGHT_COLOR);
        }
    }
    for (int i = 0; i < g->num_nodes; i++) {
        DrawCircleV(pos[i], NODE_RADIUS, NODE_COLOR);
        DrawCircleLinesV(pos[i], NODE_RADIUS, DARKGRAY);
        char label[16]; snprintf(label,sizeof(label),"%d",i);
        int tw=MeasureText(label,18);
        DrawText(label,(int)pos[i].x-tw/2,(int)pos[i].y-9,18,WHITE);
    }
}

/* ──────────────────────────────────────────────
   Message queue
   ────────────────────────────────────────────── */

#define MAX_QUEUE 256

typedef struct {
    IPCMessage msgs[MAX_QUEUE];
    int head, tail, count;
} MsgQueue;

static void queue_push(MsgQueue *q, IPCMessage msg) {
    if (q->count >= MAX_QUEUE) return;
    q->msgs[q->tail] = msg;
    q->tail = (q->tail + 1) % MAX_QUEUE;
    q->count++;
}

static int queue_pop(MsgQueue *q, IPCMessage *out) {
    if (q->count == 0) return 0;
    *out = q->msgs[q->head];
    q->head = (q->head + 1) % MAX_QUEUE;
    q->count--;
    return 1;
}

/* ──────────────────────────────────────────────
   Per-traveler visual state
   ────────────────────────────────────────────── */

typedef enum {
    VIS_IDLE,
    VIS_MOVING,
    VIS_WAITING,   /* waiting outside node — shown in yellow */
    VIS_INSIDE,    /* inside node */
    VIS_DONE
} VisStatus;

typedef struct {
    Vector2   pos;
    Vector2   from;
    Vector2   to;
    float     t;
    float     speed;
    VisStatus status;
    MsgQueue  queue;
    int       current_node;
} VisState;

/* ──────────────────────────────────────────────
   Draw play/stop button
   ────────────────────────────────────────────── */

static void draw_button(int playing, int W) {
    Rectangle btn={(float)(W-120),10,110,36};
    Color col=playing?BTN_STOP_COLOR:BTN_COLOR;
    const char *label=playing?"STOP":"PLAY";
    DrawRectangleRec(btn,col);
    DrawRectangleLinesEx(btn,2,WHITE);
    int tw=MeasureText(label,18);
    DrawText(label,(int)(btn.x+(btn.width-tw)/2),(int)(btn.y+9),18,WHITE);
}

/* ──────────────────────────────────────────────
   Main GUI
   ────────────────────────────────────────────── */

void run_gui(SimData *data) {
    const int W = WINDOW_WIDTH;
    const int H = WINDOW_HEIGHT;
    int N = data->graph->num_nodes;
    int K = data->num_travelers;

    Vector2 *pos = malloc(sizeof(Vector2) * N);
    compute_layout(N, pos, W, H);

    VisState *vs = calloc(K, sizeof(VisState));
    for (int i = 0; i < K; i++) {
        vs[i].pos          = pos[data->travelers[i].src];
        vs[i].from         = pos[data->travelers[i].src];
        vs[i].to           = pos[data->travelers[i].src];
        vs[i].t            = 1.0f;
        vs[i].speed        = 1.5f;
        vs[i].status       = VIS_IDLE;
        vs[i].current_node = data->travelers[i].src;
        vs[i].queue.head = vs[i].queue.tail = vs[i].queue.count = 0;
    }

    int playing = 0;

    InitWindow(W, H, "OS Project - Synchronized Simulation");
    SetTargetFPS(60);

    if (!IsWindowReady()) {
        fprintf(stderr, "Error: window failed\n");
        free(pos); free(vs); return;
    }

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        /* Button */
        Rectangle btn_rect={(float)(W-120),10,110,36};
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            CheckCollisionPointRec(GetMousePosition(), btn_rect)) {
            playing = !playing;
        }

        /* Poll pipes */
        if (playing) {
            for (int i = 0; i < K; i++) {
                if (vs[i].status == VIS_DONE) continue;
                if (data->travelers[i].pipe_fd < 0) continue;
                IPCMessage msg;
                ssize_t n = read(data->travelers[i].pipe_fd, &msg, sizeof(msg));
                if (n == (ssize_t)sizeof(msg)) {
                    /* Print log */
                    switch (msg.msg_type) {
                        case MSG_WAITING:
                            printf("[PID=%d] waiting outside node %d\n",
                                   (int)msg.pid, msg.current_node);
                            break;
                        case MSG_ENTERING:
                            if (msg.next_node == -1)
                                printf("[PID=%d] entered node %d | next: DESTINATION\n",
                                       (int)msg.pid, msg.current_node);
                            else
                                printf("[PID=%d] entered node %d | next: %d\n",
                                       (int)msg.pid, msg.current_node, msg.next_node);
                            break;
                        case MSG_DONE:
                            printf("[PID=%d] arrived at node %d | DESTINATION\n",
                                   (int)msg.pid, msg.current_node);
                            break;
                        default: break;
                    }
                    fflush(stdout);
                    queue_push(&vs[i].queue, msg);
                }
            }
        }

        /* Update visual states */
        for (int i = 0; i < K; i++) {
            if (vs[i].status == VIS_DONE) continue;

            /* Animate movement */
            if (vs[i].status == VIS_MOVING) {
                vs[i].t += vs[i].speed * dt;
                if (vs[i].t >= 1.0f) {
                    vs[i].t   = 1.0f;
                    vs[i].pos = vs[i].to;
                    vs[i].status = VIS_IDLE;
                } else {
                    vs[i].pos.x = vs[i].from.x + vs[i].t*(vs[i].to.x-vs[i].from.x);
                    vs[i].pos.y = vs[i].from.y + vs[i].t*(vs[i].to.y-vs[i].from.y);
                }
            }

            /* Consume next message when not moving */
            if (vs[i].status == VIS_IDLE || vs[i].status == VIS_WAITING ||
                vs[i].status == VIS_INSIDE) {
                IPCMessage msg;
                if (queue_pop(&vs[i].queue, &msg)) {
                    switch (msg.msg_type) {
                        case MSG_WAITING:
                            vs[i].status       = VIS_WAITING;
                            vs[i].current_node = msg.current_node;
                            break;
                        case MSG_ENTERING:
                            vs[i].status       = VIS_INSIDE;
                            vs[i].current_node = msg.current_node;
                            vs[i].pos          = pos[msg.current_node];
                            if (msg.next_node >= 0) {
                                /* Start moving to next node */
                                vs[i].from   = pos[msg.current_node];
                                vs[i].to     = pos[msg.next_node];
                                vs[i].t      = 0.0f;
                                vs[i].status = VIS_MOVING;
                            }
                            break;
                        case MSG_DONE:
                            vs[i].status = VIS_DONE;
                            vs[i].pos    = pos[msg.current_node];
                            break;
                        default: break;
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(BG_COLOR);
        DrawText("Synchronized Simulation (Semaphores)", 10, 10, 18, TEXT_COLOR);

        draw_graph(data->graph, pos);
        draw_button(playing, W);

        /* Draw travelers */
        int all_done = 1;
        for (int i = 0; i < K; i++) {
            if (vs[i].status != VIS_DONE) all_done = 0;

            /* Color based on status */
            Color col;
            if (vs[i].status == VIS_WAITING) {
                col = WAITING_COLOR;  /* yellow = waiting */
            } else if (vs[i].status == VIS_DONE) {
                col = DONE_COLOR;     /* gray = done */
            } else {
                col = TRAVELER_COLORS[i % TRAVELER_COLOR_COUNT];
            }

            DrawCircleV(vs[i].pos, NODE_RADIUS * 0.6f, col);
            DrawCircleLinesV(vs[i].pos, NODE_RADIUS * 0.6f, WHITE);
            char tlabel[16]; snprintf(tlabel,sizeof(tlabel),"T%d",i+1);
            int tw=MeasureText(tlabel,12);
            DrawText(tlabel,(int)vs[i].pos.x-tw/2,(int)vs[i].pos.y-6,12,WHITE);
        }

        /* Legend */
        int ly=50;
        for (int i=0;i<K;i++){
            Color col = TRAVELER_COLORS[i%TRAVELER_COLOR_COUNT];
            DrawCircleV((Vector2){20,(float)ly},8,col);
            const char *status_str="";
            if(vs[i].status==VIS_WAITING) status_str=" [WAITING]";
            else if(vs[i].status==VIS_INSIDE) status_str=" [IN NODE]";
            else if(vs[i].status==VIS_DONE) status_str=" [DONE]";
            char leg[64];
            snprintf(leg,sizeof(leg),"T%d: %d->%d (pid %d)%s",
                     i+1,data->travelers[i].src,data->travelers[i].dst,
                     (int)data->travelers[i].pid, status_str);
            DrawText(leg,34,ly-6,13,TEXT_COLOR);
            ly+=22;
        }

        /* Legend for colors */
        DrawCircleV((Vector2){20,(float)(H-70)},8,WAITING_COLOR);
        DrawText("Waiting outside node",34,H-76,13,TEXT_COLOR);
        DrawCircleV((Vector2){20,(float)(H-50)},8,TRAVELER_COLORS[0]);
        DrawText("Moving / Inside node",34,H-56,13,TEXT_COLOR);
        DrawCircleV((Vector2){20,(float)(H-30)},8,DONE_COLOR);
        DrawText("Arrived at destination",34,H-36,13,TEXT_COLOR);

        /* All done */
        if (all_done && playing) {
            const char *msg="All travelers arrived!";
            int tw=MeasureText(msg,28);
            DrawRectangle(W/2-tw/2-16,H/2-30,tw+32,56,CLITERAL(Color){0,0,0,180});
            DrawText(msg,W/2-tw/2,H/2-16,28,GREEN);
        }

        EndDrawing();
    }

    free(pos); free(vs);
    CloseWindow();
}