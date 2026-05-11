#ifndef GUI_H
#define GUI_H

#include "graph.h"
#include <raylib.h>

/* ── Window ── */
#define WINDOW_WIDTH  900
#define WINDOW_HEIGHT 700

/* ── Node appearance ── */
#define NODE_RADIUS 22.0f

/* ── Colors ── */
#define BG_COLOR     CLITERAL(Color){ 30,  30,  40,  255 }  /* dark background   */
#define NODE_COLOR   CLITERAL(Color){ 70,  130, 180, 255 }  /* steel blue        */
#define EDGE_COLOR   CLITERAL(Color){ 160, 160, 160, 255 }  /* light gray        */
#define PATH_COLOR   CLITERAL(Color){ 255, 165, 0,   255 }  /* orange highlight  */
#define WEIGHT_COLOR CLITERAL(Color){ 200, 200, 100, 255 }  /* yellow-ish        */
#define TEXT_COLOR   CLITERAL(Color){ 220, 220, 220, 255 }  /* off-white         */

/* Function declarations */
void compute_layout(int num_nodes, Vector2 *positions, int width, int height);
void run_gui(Graph *g, const DijkstraResult *result, int src, int dst);

#endif /* GUI_H */

