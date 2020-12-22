#include <common.h>
#include <stdio.h>
#include <stdlib.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxpad.h>

int SCR_W = 320;
int SCR_H = 240;
#define OT_LEN  2

static DISPENV disp[2];
static DRAWENV draw[2];

char pribuff[2][2048];			/* Primitive packet buffers */
unsigned int ot[2][OT_LEN];		/* Ordering tables */
char *nextpri;					/* Pointer to next packet buffer offset */
int dbActive;

int angle = 0;

// Init function
void init(void) {
	ResetGraph(0);
    SetVideoMode(MODE_NTSC);
		
    SetDefDispEnv(&disp[0], 0,     0, SCR_W, SCR_H);
    SetDefDrawEnv(&draw[0], SCR_W, 0, SCR_W, SCR_H);

    SetDefDrawEnv(&draw[1], 0,     0, SCR_W, SCR_H);
    SetDefDispEnv(&disp[1], SCR_W, 0, SCR_W, SCR_H);
	
    for (int i = 0; i<2; i++){
	    setRGB0(&draw[i], 255, 255, 255);
	    draw[i].isbg = 1;
    }

	// Clear double buffer counter
	dbActive = 0;
	
	// Apply the GPU environments
	PutDispEnv(&disp[dbActive]);
	PutDrawEnv(&draw[dbActive]);

    ClearOTagR( ot[0], OT_LEN );
    ClearOTagR( ot[1], OT_LEN );
    nextpri = pribuff[dbActive];	
}

// Display function
void display(void)
{
	DrawSync(0);
	VSync(0);

	dbActive = !dbActive;
    nextpri = pribuff[dbActive];	
    ClearOTagR( ot[dbActive], OT_LEN );

	PutDrawEnv(&draw[dbActive]);
	PutDispEnv(&disp[dbActive]);

    SetDispMask(1);
    DrawOTag(ot[1-dbActive] + OT_LEN-1);
}

struct Vertex {
    int x, y;
};

void translate(struct Vertex* v, struct Vertex tr) {
    v->x += tr.x;
    v->y += tr.y;
}

void rot2d(struct Vertex* v, int angle) {
    #define M_PI 3.14
    int x = v->x;
    int y = v->y;

    int c = icos(angle);
    int s = isin(angle);
    
    v->x = (x * c - y * s )>>12;
    v->y = (x * s + y * c )>>12;
}

void drawTriangle(int cx, int cy, int size)
{
    struct Vertex v[3];
    v[0].x = -0.866f * size; v[0].y = -0.5f * size;
    v[1].x = 0.866f * size; v[1].y = -0.5f * size;
    v[2].x = 0;         v[2].y = size;

    struct Vertex tr = {
        .x = cx,
        .y = cy
    };
    for (int i = 0; i<3; i++) {
        rot2d(&v[i], angle);
        translate(&v[i], tr);
    }

    POLY_G3* p = (POLY_G3*)nextpri;
    setPolyG3(p);
    setRGB0(p, 255, 0, 0);
    setRGB1(p, 0, 255, 0);
    setRGB2(p, 0, 0, 255);
    setXY3(p,
        v[0].x, v[0].y,
        v[1].x, v[1].y,
        v[2].x, v[2].y
    );
    addPrim(ot[dbActive] + (OT_LEN-1), p);

    nextpri += sizeof(POLY_G3);

    angle += 4096/360;
    if (angle >=4096) angle = 0;
}

int main(){
    init();
    printf("\ngpu/animated-triangle\n");

    for (;;) {
        drawTriangle(SCR_W/2, SCR_H/2, SCR_H/2);        

        display();
    }
    return 0;
}
