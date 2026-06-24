#include "graph.h"
#include "gui.h"
#include <raylib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ── Layout ── */
void compute_layout(int num_nodes, Vector2 *positions, int width, int height) {
    float cx=width/2.0f, cy=height/2.0f;
    float radius=(float)(width<height?width:height)*0.35f;
    for(int i=0;i<num_nodes;i++){
        float angle=(2.0f*PI*i)/num_nodes-PI/2.0f;
        positions[i].x=cx+radius*cosf(angle);
        positions[i].y=cy+radius*sinf(angle);
    }
}

/* ── Draw arrow ── */
static void draw_arrow(Vector2 from, Vector2 to, Color color) {
    float dx=to.x-from.x,dy=to.y-from.y;
    float len=sqrtf(dx*dx+dy*dy); if(len<1.0f)return;
    float ux=dx/len,uy=dy/len;
    Vector2 start={from.x+ux*NODE_RADIUS,from.y+uy*NODE_RADIUS};
    Vector2 end={to.x-ux*NODE_RADIUS,to.y-uy*NODE_RADIUS};
    DrawLineEx(start,end,2.0f,color);
    float as=12.0f,angle=atan2f(uy,ux);
    Vector2 tip=end;
    Vector2 p1={tip.x+as*cosf(angle+PI*0.85f),tip.y+as*sinf(angle+PI*0.85f)};
    Vector2 p2={tip.x+as*cosf(angle-PI*0.85f),tip.y+as*sinf(angle-PI*0.85f)};
    DrawTriangle(tip,p1,p2,color);
}

/* ── Draw graph ── */
static void draw_graph(Graph *g, Vector2 *pos) {
    for(int u=0;u<g->num_nodes;u++){
        for(AdjNode *e=g->adj[u];e;e=e->next){
            draw_arrow(pos[u],pos[e->dest],EDGE_COLOR);
            float mx=(pos[u].x+pos[e->dest].x)/2.0f;
            float my=(pos[u].y+pos[e->dest].y)/2.0f;
            char wlabel[16]; snprintf(wlabel,sizeof(wlabel),"%d",e->weight);
            DrawText(wlabel,(int)mx+4,(int)my-10,14,WEIGHT_COLOR);
        }
    }
    for(int i=0;i<g->num_nodes;i++){
        DrawCircleV(pos[i],NODE_RADIUS,NODE_COLOR);
        DrawCircleLinesV(pos[i],NODE_RADIUS,DARKGRAY);
        char label[16]; snprintf(label,sizeof(label),"%d",i);
        int tw=MeasureText(label,18);
        DrawText(label,(int)pos[i].x-tw/2,(int)pos[i].y-9,18,WHITE);
    }
}

/* ── Message queue ── */
#define MAX_QUEUE 256
typedef struct {
    IPCMessage msgs[MAX_QUEUE];
    int head,tail,count;
} MsgQueue;

static void mq_push(MsgQueue *q, IPCMessage m){
    if(q->count>=MAX_QUEUE)return;
    q->msgs[q->tail]=m; q->tail=(q->tail+1)%MAX_QUEUE; q->count++;
}
static int mq_pop(MsgQueue *q, IPCMessage *out){
    if(q->count==0)return 0;
    *out=q->msgs[q->head]; q->head=(q->head+1)%MAX_QUEUE; q->count--;
    return 1;
}

/* ── Visual state ── */
typedef enum { VIS_IDLE,VIS_MOVING,VIS_WAITING,VIS_INSIDE,VIS_DONE } VisStatus;
typedef struct {
    Vector2 pos,from,to;
    float t,speed;
    VisStatus status;
    MsgQueue queue;
    int current_node;
} VisState;

/* ── Button ── */
static void draw_button(int playing, int W) {
    Rectangle btn={(float)(W-120),10,110,36};
    Color col=playing?BTN_STOP_COLOR:BTN_COLOR;
    const char *label=playing?"STOP":"PLAY";
    DrawRectangleRec(btn,col); DrawRectangleLinesEx(btn,2,WHITE);
    int tw=MeasureText(label,18);
    DrawText(label,(int)(btn.x+(btn.width-tw)/2),(int)(btn.y+9),18,WHITE);
}

/* ── try_grant: give next queued traveler access to node ── */
static void try_grant(int node, SimData *data, NodeQueue *node_queues) {
    if(node_queues[node].occupied) return;
    if(!node_queues[node].head) return;
    int idx = node_queues[node].head->traveler_idx;
    WaitEntry *old = node_queues[node].head;
    node_queues[node].head = old->next;
    free(old);
    node_queues[node].occupied = 1;
    IPCMessage grant; grant.msg_type=MSG_GRANT; grant.current_node=node;
    write(data->travelers[idx].grant_write, &grant, sizeof(grant));
}

/* ── Main GUI ── */
void run_gui(SimData *data, NodeQueue *node_queues) {
    const int W=WINDOW_WIDTH, H=WINDOW_HEIGHT;
    int N=data->graph->num_nodes, K=data->num_travelers;
    const char *sched_name = (data->sched_algo==SCHED_FCFS) ? "FCFS" : "SJF";

    Vector2 *pos=malloc(sizeof(Vector2)*N);
    compute_layout(N,pos,W,H);

    VisState *vs=calloc(K,sizeof(VisState));
    for(int i=0;i<K;i++){
        vs[i].pos=pos[data->travelers[i].src];
        vs[i].from=vs[i].to=pos[data->travelers[i].src];
        vs[i].t=1.0f; vs[i].speed=1.5f;
        vs[i].status=VIS_IDLE;
        vs[i].current_node=data->travelers[i].src;
        vs[i].queue.head=vs[i].queue.tail=vs[i].queue.count=0;
    }

    int playing=0;

    InitWindow(W,H,"OS Project - Scheduling Simulation");
    SetTargetFPS(60);

    if(!IsWindowReady()){
        fprintf(stderr,"Error: window failed\n");
        free(pos); free(vs); return;
    }

    while(!WindowShouldClose()){
        float dt=GetFrameTime();

        /* Button */
        Rectangle btn_rect={(float)(W-120),10,110,36};
        if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&
           CheckCollisionPointRec(GetMousePosition(),btn_rect))
            playing=!playing;

        /* Poll pipes */
        if(playing){
            for(int i=0;i<K;i++){
                if(vs[i].status==VIS_DONE) continue;
                IPCMessage msg;
                ssize_t n=read(data->travelers[i].pipe_read,&msg,sizeof(msg));
                if(n==(ssize_t)sizeof(msg)){
                    switch(msg.msg_type){
                        case MSG_REQUEST:
                            printf("[PID=%d] waiting outside node %d (%s)\n",
                                   (int)msg.pid, msg.current_node, sched_name);
                            fflush(stdout);
                            /* Add to node queue */
                            {
                                WaitEntry *entry=malloc(sizeof(WaitEntry));
                                entry->traveler_idx=i;
                                entry->remaining_path=msg.remaining_path;
                                entry->arrival_time=msg.arrival_time;
                                entry->next=NULL;
                                if(data->sched_algo==SCHED_FCFS){
                                    /* Append to end */
                                    WaitEntry **cur=&node_queues[msg.current_node].head;
                                    while(*cur) cur=&(*cur)->next;
                                    *cur=entry;
                                } else {
                                    /* SJF: insert by remaining path */
                                    WaitEntry **cur=&node_queues[msg.current_node].head;
                                    while(*cur&&(*cur)->remaining_path<=entry->remaining_path)
                                        cur=&(*cur)->next;
                                    entry->next=*cur; *cur=entry;
                                }
                                try_grant(msg.current_node,data,node_queues);
                            }
                            mq_push(&vs[i].queue,msg);
                            break;
                        case MSG_ENTERING:
                            printf("[PID=%d] entered node %d | next: ",
                                   (int)msg.pid, msg.current_node);
                            if(msg.next_node==-1) printf("DESTINATION\n");
                            else printf("%d\n",msg.next_node);
                            fflush(stdout);
                            mq_push(&vs[i].queue,msg);
                            break;
                        case MSG_LEAVING:
                            /* Node is now free — grant to next in queue */
                            node_queues[msg.current_node].occupied=0;
                            try_grant(msg.current_node,data,node_queues);
                            break;
                        case MSG_DONE:
                            printf("[PID=%d] arrived at node %d | DESTINATION\n",
                                   (int)msg.pid, msg.current_node);
                            fflush(stdout);
                            mq_push(&vs[i].queue,msg);
                            break;
                        default: break;
                    }
                }
            }
        }

        /* Update visuals */
        for(int i=0;i<K;i++){
            if(vs[i].status==VIS_DONE) continue;
            if(vs[i].status==VIS_MOVING){
                vs[i].t+=vs[i].speed*dt;
                if(vs[i].t>=1.0f){
                    vs[i].t=1.0f; vs[i].pos=vs[i].to; vs[i].status=VIS_IDLE;
                } else {
                    vs[i].pos.x=vs[i].from.x+vs[i].t*(vs[i].to.x-vs[i].from.x);
                    vs[i].pos.y=vs[i].from.y+vs[i].t*(vs[i].to.y-vs[i].from.y);
                }
            }
            if(vs[i].status!=VIS_MOVING){
                IPCMessage msg;
                if(mq_pop(&vs[i].queue,&msg)){
                    switch(msg.msg_type){
                        case MSG_REQUEST:
                            vs[i].status=VIS_WAITING;
                            vs[i].current_node=msg.current_node;
                            break;
                        case MSG_ENTERING:
                            vs[i].pos=pos[msg.current_node];
                            if(msg.next_node==-1){
                                vs[i].status=VIS_INSIDE;
                            } else {
                                vs[i].from=pos[msg.current_node];
                                vs[i].to=pos[msg.next_node];
                                vs[i].t=0.0f; vs[i].status=VIS_MOVING;
                            }
                            break;
                        case MSG_DONE:
                            vs[i].status=VIS_DONE;
                            vs[i].pos=pos[msg.current_node];
                            break;
                        default: break;
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(BG_COLOR);

        /* Title with scheduler name */
        char title[64];
        snprintf(title,sizeof(title),"Scheduling Simulation [%s]",sched_name);
        DrawText(title,10,10,18,TEXT_COLOR);

        draw_graph(data->graph,pos);
        draw_button(playing,W);

        /* Draw travelers */
        int all_done=1;
        /* Count waiters per node to offset them */
        int wait_count[MAX_NODES] = {0};
        for(int i=0;i<K;i++){
            if(vs[i].status==VIS_WAITING)
                wait_count[vs[i].current_node]++;
        }
        int wait_drawn[MAX_NODES] = {0};

        for(int i=0;i<K;i++){
            if(vs[i].status!=VIS_DONE) all_done=0;
            Color col;
            if(vs[i].status==VIS_WAITING)     col=WAITING_COLOR;
            else if(vs[i].status==VIS_DONE)   col=DONE_COLOR;
            else                               col=TRAVELER_COLORS[i%TRAVELER_COLOR_COUNT];

            Vector2 draw_pos = vs[i].pos;

            /* Offset waiting travelers around their node */
            if(vs[i].status==VIS_WAITING){
                int node = vs[i].current_node;
                int slot = wait_drawn[node]++;
                int total = wait_count[node];
                float offset_angle = (2.0f * PI * slot / total) + PI/4.0f;
                float offset_dist  = NODE_RADIUS * 1.8f;
                draw_pos.x = pos[node].x + offset_dist * cosf(offset_angle);
                draw_pos.y = pos[node].y + offset_dist * sinf(offset_angle);
            }

            DrawCircleV(draw_pos,NODE_RADIUS*0.6f,col);
            DrawCircleLinesV(draw_pos,NODE_RADIUS*0.6f,WHITE);
            char tlabel[16]; snprintf(tlabel,sizeof(tlabel),"T%d",i+1);
            int tw=MeasureText(tlabel,12);
            DrawText(tlabel,(int)draw_pos.x-tw/2,(int)draw_pos.y-6,12,WHITE);
        }

        /* Legend */
        int ly=50;
        for(int i=0;i<K;i++){
            Color col=TRAVELER_COLORS[i%TRAVELER_COLOR_COUNT];
            DrawCircleV((Vector2){20,(float)ly},8,col);
            const char *st="";
            if(vs[i].status==VIS_WAITING) st=" [WAITING]";
            else if(vs[i].status==VIS_INSIDE) st=" [IN NODE]";
            else if(vs[i].status==VIS_DONE) st=" [DONE]";
            char leg[64];
            snprintf(leg,sizeof(leg),"T%d: %d->%d (pid %d)%s",
                     i+1,data->travelers[i].src,data->travelers[i].dst,
                     (int)data->travelers[i].pid,st);
            DrawText(leg,34,ly-6,13,TEXT_COLOR);
            ly+=22;
        }

        /* Color legend */
        DrawCircleV((Vector2){20,(float)(H-70)},8,WAITING_COLOR);
        DrawText("Waiting (scheduled)",34,H-76,13,TEXT_COLOR);
        DrawCircleV((Vector2){20,(float)(H-50)},8,TRAVELER_COLORS[0]);
        DrawText("Moving / Inside node",34,H-56,13,TEXT_COLOR);
        DrawCircleV((Vector2){20,(float)(H-30)},8,DONE_COLOR);
        DrawText("Arrived",34,H-36,13,TEXT_COLOR);

        if(all_done&&playing){
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