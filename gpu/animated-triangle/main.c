#include <common.h>
#include <stdio.h>
#include <stdlib.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxpad.h>
#include <psxgte.h>
#include <inline_c.h>

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

#define USE_GTE

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
        draw[i].dfe=1;
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

void rot2d(struct DVECTOR* v, int angle) {
    int x = v->vx;
    int y = v->vy;

    int c = icos(angle);
    int s = isin(angle);
    
    v->vx = (x * c - y * s ) / ONE;
    v->vy = (x * s + y * c ) / ONE;
}

void gte_rot2d(struct DVECTOR* v, int angle) {
    SVECTOR rotationVec = { 0 };
    VECTOR translationVec = { 0 };
    MATRIX m;

    rotationVec.vz = angle;

    RotMatrix(&rotationVec, &m);
    TransMatrix(&m, &translationVec);
    gte_SetRotMatrix(&m); // Matrix could be setup once
    gte_SetTransMatrix(&m);
    
    v->vx /= 2;
    v->vy /= 2;
    gte_ldv0(v); 
    gte_rtps(); // Rotate
    gte_stsxy(v);
}

void drawTriangle(int cx, int cy, int size)
{
    struct DVECTOR v[3] = {0};
    v[0].vx = -0.866f * size; v[0].vy = -0.5f * size;
    v[1].vx =  0.866f * size; v[1].vy = -0.5f * size;
    v[2].vx =      0;         v[2].vy = size;

    for (int i = 0; i<3; i++) {
    #ifdef USE_GTE
        gte_rot2d(&v[i], angle);
    #else
        rot2d(&v[i], angle);
    #endif
    }

    POLY_G3* p = (POLY_G3*)nextpri;
    setPolyG3(p);
    setRGB0(p, 255, 0, 0);
    setRGB1(p, 0, 255, 0);
    setRGB2(p, 0, 0, 255);
    setXY3(p,
        cx + v[0].vx, cy + v[0].vy,
        cx + v[1].vx, cy + v[1].vy,
        cx + v[2].vx, cy + v[2].vy
    );
    addPrim(ot[dbActive] + (OT_LEN-1), p);

    nextpri += sizeof(POLY_G3);

    angle += ONE/360;
    if (angle >= ONE) angle = 0;
}

int main(){
    init();
    printf("\ngpu/animated-triangle\n");

#ifdef USE_GTE
    printf("Initializing GTE... ");
    InitGeom();
    printf("done.\n");
	gte_SetGeomOffset(0, 0);
#endif

    for (;;) {
        drawTriangle(SCR_W/2, SCR_H/2, SCR_H/2);        

        display();
    }
    return 0;
}
