#include "neslib.h"

static unsigned char i, spr;
static unsigned char dx, dy;
static unsigned char gx, gy;

const unsigned char pal[32]={
    // Bg
    0x21, 0x11, 0x26, 0x15,
    0x21, 0x30, 0x26, 0x0F,
    0x21, 0x11, 0x26, 0x38,
    0x21, 0x11, 0x26, 0x38,

    // Sprites
    0x21, 0x11, 0x26, 0x15,
    0x21, 0x30, 0x26, 0x0F,
    0x21, 0x17, 0x37, 0x0D,
    0x21, 0x17, 0x37, 0x11
};

#define G(x) (x)*8

void main(void) {
    pal_all(pal);
    ppu_on_all();

    // initial position
    dx = (256 - 32) / 2;
    dy = (240 - 16) / 2;

    gx = dx;
    gy = dy + G(4);

    while (1) {
        ppu_wait_frame();

        i = pad_poll(0);

        if((i & PAD_LEFT)  && dx >   0) dx-=2;
        if((i & PAD_RIGHT) && dx < 248) dx+=2;
        if((i & PAD_UP)    && dy >   0) dy-=2;
        if((i & PAD_DOWN)  && dy < 232) dy+=2;

        spr = 0;

        // dog
        spr = oam_spr(dx+G(2), dy+G(0),  2, 0, spr); // hat
        spr = oam_spr(dx+G(0), dy+G(1), 16, 0, spr); // tail
        spr = oam_spr(dx+G(2), dy+G(1), 18, 1, spr); // head
        spr = oam_spr(dx+G(3), dy+G(1), 19, 1, spr); // nose
        spr = oam_spr(dx+G(0), dy+G(2), 32, 0, spr); // back
        spr = oam_spr(dx+G(1), dy+G(2), 33, 0, spr); // body
        spr = oam_spr(dx+G(2), dy+G(2), 34, 0, spr); // neck
        //spr = oam_spr(x+G(3), y+G(2), 35, 0, spr); // mouth

        // girl
        spr = oam_spr(gx+G(0), gy+G(0),  4, 2, spr); // head
        spr = oam_spr(gx+G(1), gy+G(0),  5, 2, spr); // head
        spr = oam_spr(gx+G(0), gy+G(1), 20, 3, spr); // left arm
        spr = oam_spr(gx+G(1), gy+G(1), 21, 3, spr); // body
        spr = oam_spr(gx+G(0), gy+G(2), 36, 3, spr); // suitcase
        spr = oam_spr(gx+G(1), gy+G(2), 37, 3, spr); // legs
    };
}
