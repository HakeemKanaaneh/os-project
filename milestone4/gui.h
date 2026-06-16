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
#define STEP_TIME  0.30f
#define NODE_WAIT  1.00f

/* ── Colors ── */
#define BG_COLOR       CLITERAL(Color){ 30,  30,  40,  255 }
#define NODE_COLOR     CLITERAL(Color){ 70,  130, 180, 255 }
#define EDGE_COLOR     CLITERAL(Color){ 160, 160, 160, 255 }
#define PATH_COLOR     CLITERAL(Color){ 255, 165, 0,   255 }
#define WEIGHT_COLOR   CLITERAL(Color){ 200, 200, 100, 255 }
#define TEXT_COLOR     CLITERAL(Color){ 220, 220, 220, 255 }
#define BTN_COLOR      CLITERAL(Color){ 60,  180,  60, 255 }
#define BTN_STOP_COLOR CLITERAL(Color){ 200,  60,  60, 255 }

/* ── Traveler colors (up to MAX_TRAVELERS) ── */
#define TRAVELER_COLOR_COUNT 8
static const Color TRAVELER_COLORS[] = {
    {255,  80,  80, 255},  /* red     */
    { 80, 200,  80, 255},  /* green   */
    { 80, 140, 255, 255},  /* blue    */
    {255, 220,  50, 255},  /* yellow  */
    {220,  80, 220, 255},  /* magenta */
    { 80, 220, 220, 255},  /* cyan    */
    {255, 160,  40, 255},  /* orange  */
    {180, 255, 100, 255},  /* lime    */
};

/* ── Animation states ── */
typedef enum {
    ANIM_IDLE,
    ANIM_MOVING,
    ANIM_WAITING,
    ANIM_DONE
} AnimState;

/* ── Per-traveler animation context ── */
typedef struct {
    AnimState state;
    int       path_idx;
    int       step;
    int       total_steps;
    float     timer;
    Vector2   entity_pos;
} TravelerAnim;

/* Function declarations */
void compute_layout(int num_nodes, Vector2 *positions, int width, int height);
void run_gui(SimData *data);

#endif /* GUI_H */
