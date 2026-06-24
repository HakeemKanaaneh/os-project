#ifndef GUI_H
#define GUI_H

#include "graph.h"
#include <raylib.h>

#define WINDOW_WIDTH  900
#define WINDOW_HEIGHT 700
#define NODE_RADIUS   22.0f

#define BG_COLOR       CLITERAL(Color){ 30,  30,  40,  255 }
#define NODE_COLOR     CLITERAL(Color){ 70,  130, 180, 255 }
#define EDGE_COLOR     CLITERAL(Color){ 160, 160, 160, 255 }
#define WEIGHT_COLOR   CLITERAL(Color){ 200, 200, 100, 255 }
#define TEXT_COLOR     CLITERAL(Color){ 220, 220, 220, 255 }
#define BTN_COLOR      CLITERAL(Color){  60, 180,  60, 255 }
#define BTN_STOP_COLOR CLITERAL(Color){ 200,  60,  60, 255 }
#define WAITING_COLOR  CLITERAL(Color){ 255, 220,   0, 255 }
#define DONE_COLOR     CLITERAL(Color){ 120, 120, 120, 255 }

#define TRAVELER_COLOR_COUNT 8
static const Color TRAVELER_COLORS[] = {
    {255,  80,  80, 255},
    { 80, 200,  80, 255},
    { 80, 140, 255, 255},
    {255, 160,  40, 255},
    {220,  80, 220, 255},
    { 80, 220, 220, 255},
    {180, 255, 100, 255},
    {255, 140, 180, 255},
};

void compute_layout(int num_nodes, Vector2 *positions, int width, int height);
void run_gui(SimData *data, NodeQueue *node_queues);

#endif /* GUI_H */
