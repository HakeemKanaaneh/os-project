#ifndef GUI_H
#define GUI_H

#include "graph.h"
#include <raylib.h>

/* ── Window ── */
#define WINDOW_WIDTH  900
#define WINDOW_HEIGHT 700

/* ── Node appearance ── */
#define NODE_RADIUS 22.0f

/* ── Animation timing ── */
#define STEP_TIME      0.30f   /* seconds per edge step (300ms) */
#define NODE_WAIT      1.00f   /* seconds to wait at intermediate nodes */

/* ── Colors ── */
#define BG_COLOR       CLITERAL(Color){ 30,  30,  40,  255 }
#define NODE_COLOR     CLITERAL(Color){ 70,  130, 180, 255 }
#define EDGE_COLOR     CLITERAL(Color){ 160, 160, 160, 255 }
#define PATH_COLOR     CLITERAL(Color){ 255, 165, 0,   255 }
#define WEIGHT_COLOR   CLITERAL(Color){ 200, 200, 100, 255 }
#define TEXT_COLOR     CLITERAL(Color){ 220, 220, 220, 255 }
#define ENTITY_COLOR   CLITERAL(Color){ 255,  80,  80, 255 }
#define BTN_COLOR      CLITERAL(Color){ 60,  180,  60, 255 }
#define BTN_STOP_COLOR CLITERAL(Color){ 200,  60,  60, 255 }

/* ── Animation states ── */
typedef enum {
    ANIM_IDLE,
    ANIM_MOVING,
    ANIM_WAITING,
    ANIM_DONE
} AnimState;

/* ── Animation context ── */
typedef struct {
    AnimState state;
    int       playing;
    int       path_idx;
    int       step;
    int       total_steps;
    float     timer;
    Vector2   entity_pos;
} AnimContext;

/* Function declarations */
void compute_layout(int num_nodes, Vector2 *positions, int width, int height);
void run_gui(Graph *g, const DijkstraResult *result, int src, int dst);

#endif /* GUI_H */
