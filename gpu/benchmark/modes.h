#pragma once
#include <common.h>
#include "type_traits.hpp"

#define SCR_W 320
#define SCR_H 240

#define PACKET_LEN 65535

extern uint32_t drawListBuffer[PACKET_LEN];
extern uint32_t drawListOt;
extern uint32_t* ptr;

extern int semiTransparency;
extern bool dithering;
extern TIM_IMAGE texture;

extern uint32_t pixelsDrawn;

#define ASSERT(x, msg) do { \
    if (!(x)) { \
        printf("ASSERTION FAILED: "#msg"\n"); \
        __asm__ volatile (".word 0xFC000000"); \
    } \
} while (0)

void advanceRand(int n);

template<typename T>
void addToDrawList(T* primitive) {
    addPrim(&drawListOt, primitive);
}

uint32_t triangleArea(
    int x1, int y1,
    int x2, int y2,
    int x3, int y3
) {
    int area = (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2)) / 2; 
    if (area < 0) return -area;
    else return area;
}

uint32_t sqrt(uint32_t val) {
    uint32_t temp, g=0, b = 0x8000, bshft = 15;
    do {
        if (val >= (temp = (((g << 1) + b)<<bshft--))) {
           g += b;
           val -= temp;
        }
    } while (b >>= 1);
    return g;
}

uint32_t lineLength(
    int x1, int y1,
    int x2, int y2
) {
    int x = x2 - x1;
    int y = y2 - y1;
    return sqrt(x*x + y*y);
}

template<typename T, bool shaded = false>
void drawPolygons(int n) {
    pixelsDrawn = 0;
    for (int i = 0; i < n; i++) {
        T *p = (T*)ptr;
        ptr += sizeof(T);

        constexpr bool isTriangle = IS_SAME(T, POLY_F3)  || IS_SAME(T, POLY_G3) || IS_SAME(T, POLY_FT3) || IS_SAME(T, POLY_GT3);
        constexpr bool isQuad     = IS_SAME(T, POLY_F4)  || IS_SAME(T, POLY_G4) || IS_SAME(T, POLY_FT4) || IS_SAME(T, POLY_GT4);
        constexpr bool has3Colors = IS_SAME(T, POLY_G3)  || IS_SAME(T, POLY_GT3);
        constexpr bool has4Colors = IS_SAME(T, POLY_G4)  || IS_SAME(T, POLY_GT4);
        constexpr bool isTextured = IS_SAME(T, POLY_FT3) || IS_SAME(T, POLY_GT3) || IS_SAME(T, POLY_FT4) || IS_SAME(T, POLY_GT4);

        if constexpr (IS_SAME(T, POLY_F3))  setPolyF3(p);
        else if      (IS_SAME(T, POLY_G3))  setPolyG3(p);
        else if      (IS_SAME(T, POLY_FT3)) setPolyFT3(p);
        else if      (IS_SAME(T, POLY_GT3)) setPolyGT3(p);
        else if      (IS_SAME(T, POLY_F4))  setPolyF4(p);
        else if      (IS_SAME(T, POLY_G4))  setPolyG4(p);
        else if      (IS_SAME(T, POLY_FT4)) setPolyFT4(p);
        else if      (IS_SAME(T, POLY_GT4)) setPolyGT4(p);
        else ASSERT(false, "Invalid type");

        setSemiTrans(p, semiTransparency != 0);

        p->x0 = rand() % SCR_W;  p->y0 = rand() % SCR_H;
        p->x1 = rand() % SCR_W;  p->y1 = rand() % SCR_H;
        p->x2 = rand() % SCR_W;  p->y2 = rand() % SCR_H;
        if constexpr (isQuad) {
            p->x3 = rand() % SCR_W;  p->y3 = rand() % SCR_H;
        } else {
            advanceRand(2);
        }

        pixelsDrawn += triangleArea(
            p->x0, p->y0,
            p->x1, p->y1,
            p->x2, p->y2
        );

         if constexpr (isQuad) {
            pixelsDrawn += triangleArea(
                p->x1, p->y1,
                p->x2, p->y2,
                p->x3, p->y3
            );
        }

        if constexpr (!has3Colors && !has4Colors) {
            setRGB0(p, rand()%255, rand()%255, rand()%255);
            advanceRand(9);
        } else if constexpr (has3Colors) {
            setRGB0(p, rand()%255, rand()%255, rand()%255);
            setRGB1(p, rand()%255, rand()%255, rand()%255);
            setRGB2(p, rand()%255, rand()%255, rand()%255);
            advanceRand(3);
        } else if constexpr (has4Colors) {
            setRGB0(p, rand()%255, rand()%255, rand()%255);
            setRGB1(p, rand()%255, rand()%255, rand()%255);
            setRGB2(p, rand()%255, rand()%255, rand()%255);
            setRGB3(p, rand()%255, rand()%255, rand()%255);
        }

        if constexpr (isTextured) {
            setShadeTex(p, !shaded);
            setTPage(p, texture.mode, (semiTransparency > 0) ? semiTransparency - 1 : 0, texture.prect->x, texture.prect->y);
            setClut(p, texture.crect->x, texture.crect->y);
            
            p->u0 = rand() % texture.prect->w;  p->u0 = rand() % texture.prect->h;
            p->u1 = rand() % texture.prect->w;  p->u1 = rand() % texture.prect->h;
            p->u2 = rand() % texture.prect->w;  p->u2 = rand() % texture.prect->h;
            if constexpr (isQuad) {
                p->u3 = rand() % texture.prect->w;  p->v3 = rand() % texture.prect->h;
            } else {
                advanceRand(2);
            }
        } else {
            advanceRand(8);
        }

        addToDrawList(p);
    }
}

template<typename T, bool shaded = false>
void drawRects(int n) {
    pixelsDrawn = 0;
    for (int i = 0; i < n; i++) {
        T *p = (T*)ptr;
        ptr += sizeof(T);

        constexpr bool isTextured = IS_SAME(T, SPRT);

        if constexpr (IS_SAME(T, TILE))  setTile(p);
        else if      (IS_SAME(T, SPRT))  setSprt(p);
        else    ASSERT(false, "Invalid type");

        setSemiTrans(p, semiTransparency != 0);

        p->x0 = rand() % SCR_W;  p->y0 = rand() % SCR_H;
        p->w  = rand() % SCR_W;  p->h  = rand() % SCR_H;
        advanceRand(4);

        pixelsDrawn += p->w * p->h;

        setRGB0(p, rand()%255, rand()%255, rand()%255);
        advanceRand(9);

        if constexpr (isTextured) {
            setShadeTex(p, !shaded);
            setClut(p, texture.crect->x, texture.crect->y);
    
            p->u0 = rand() % texture.prect->w;  p->u0 = rand() % texture.prect->h;
            advanceRand(6);
        } else {
            advanceRand(8);
        }

        addToDrawList(p);
    }
    
    // Reverse order
    DR_TPAGE *e = (DR_TPAGE*)ptr;
    ptr += sizeof(DR_TPAGE);
    setDrawTPage(e, 0, dithering, getTPage(texture.mode, (semiTransparency > 0) ? semiTransparency - 1 : 0, texture.prect->x, texture.prect->y));
    addToDrawList(e);
}

template<typename T>
void drawLines(int n) {
    pixelsDrawn = 0;
    for (int i = 0; i < n; i++) {
        T *p = (T*)ptr;
        ptr += sizeof(T);

        if constexpr (IS_SAME(T, LINE_F2))  setLineF2(p);
        else if      (IS_SAME(T, LINE_G2))  setLineG2(p);
        else    ASSERT(false, "Invalid type");

        setSemiTrans(p, semiTransparency != 0);

        p->x0 = rand() % SCR_W;  p->y0 = rand() % SCR_H;
        p->x1 = rand() % SCR_W;  p->y1 = rand() % SCR_H;
        advanceRand(4);

        pixelsDrawn += lineLength(
            p->x0, p->y0,
            p->x1, p->y1
        );

        setRGB0(p, rand()%255, rand()%255, rand()%255);
        if constexpr (IS_SAME(T, LINE_G2)) {
            setRGB1(p, rand()%255, rand()%255, rand()%255);
            advanceRand(6);
        } else {
            advanceRand(9);
        }

        advanceRand(8);

        addToDrawList(p);
    }
}

struct RenderingMode {
    using Func = void(*)(int);
    const char* description;
    Func func;
};

constexpr RenderingMode modes[] = {
    {"Triangle flat",              drawPolygons<POLY_F3       >},
    {"Triangle gouraud",           drawPolygons<POLY_G3       >},
    {"Triangle textured",          drawPolygons<POLY_FT3      >},
    {"Triangle textured flat",     drawPolygons<POLY_FT3, true>},
    {"Triangle textured gouraud",  drawPolygons<POLY_GT3, true>},
    {"Quad flat",                  drawPolygons<POLY_F4       >},
    {"Quad gouraud",               drawPolygons<POLY_G4       >},
    {"Quad textured",              drawPolygons<POLY_FT4      >},
    {"Quad textured flat",         drawPolygons<POLY_FT4, true>},
    {"Quad textured gouraud",      drawPolygons<POLY_GT4, true>},
    {"Rect",                       drawRects   <TILE          >},
    {"Rect textured",              drawRects   <SPRT          >},
    {"Rect textured flat",         drawRects   <SPRT,     true>},
    {"Line flat",                  drawLines   <LINE_F2       >},
    {"Line gouraud",               drawLines   <LINE_G2       >},
};

constexpr const char* semiTransparencyModesStr[] = {
    "off",
    "B/2+F/2",
    "B+F", 
    "B-F", 
    "B+F/4",
};
