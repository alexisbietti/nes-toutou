#include "neslib.h"


static unsigned char i, spr;
static unsigned char dx, dy; // player position
static unsigned char bx, by; // bone position
static unsigned char bs; // bone sprite, must be < 4

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

const unsigned char bone_sprite[4] = {
    0x06, 0x16, 0x07, 0x16
};

#define G(x) (x)*8

#define PLAYER_SPEED 2
#define PLAYER_MIN_X 8
#define PLAYER_MAX_X 96
#define PLAYER_MIN_Y 32
#define PLAYER_MAX_Y 196
#define BONE_MAX_X 248
#define BONE_SPEED 4

void main(void) {
    pal_all(pal);
    ppu_on_all();

    // initial position
    dx = PLAYER_MIN_X;
    dy = (PLAYER_MAX_Y + PLAYER_MIN_Y) / 2;
    bx = 0;
    bs = 0;

    while (1) {
        ppu_wait_frame();

        i = pad_poll(0);

        if((i & PAD_LEFT)  && dx > PLAYER_MIN_X) dx -= PLAYER_SPEED;
        if((i & PAD_RIGHT) && dx < PLAYER_MAX_X) dx += PLAYER_SPEED;
        if((i & PAD_UP)    && dy > PLAYER_MIN_Y) dy -= PLAYER_SPEED;
        if((i & PAD_DOWN)  && dy < PLAYER_MAX_Y) dy += PLAYER_SPEED;

        if(bx) bx += BONE_SPEED;
        if(bx > BONE_MAX_X) bx = 0;
        if((i & PAD_A) && bx == 0) {
            bx = dx + G(3);
            by = dy + G(1);
        }

        spr = 0;

        // draw player
        spr = oam_spr(dx+G(2), dy+G(0),  2, 0, spr); // hat
        spr = oam_spr(dx+G(0), dy+G(1), 16, 0, spr); // tail
        spr = oam_spr(dx+G(2), dy+G(1), 18, 1, spr); // head
        spr = oam_spr(dx+G(3), dy+G(1), 19, 1, spr); // nose
        spr = oam_spr(dx+G(0), dy+G(2), 32, 0, spr); // back
        spr = oam_spr(dx+G(1), dy+G(2), 33, 0, spr); // body
        spr = oam_spr(dx+G(2), dy+G(2), 34, 0, spr); // neck

        if (bx) {
            bs = (bs + 1) % 4;
            spr = oam_spr(bx, by, bone_sprite[bs], 1, spr); // bone
        } else {
            spr = oam_spr(0, 0, 0, 0, spr); // nothing
        }
    };
}
