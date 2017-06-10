#include "neslib.h"


static unsigned char i, spr;
static unsigned char dx, dy; // player position
static unsigned char bx, by; // bone position
static unsigned char bs; // bone sprite, must be < 4
static unsigned char ex, ey; // enemy position
static unsigned char es; // enemy sprite 0 = no enemy
static unsigned char r; // random number
static unsigned char score;
static unsigned char game_over;

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

const unsigned char bone_sprite[8] = {
    0x06, 0x06, 0x16, 0x16, 0x07, 0x07, 0x16, 0x16
};

const unsigned char enemy_sprite[11] = {
    0x00, // hidden
    0x08, 0x08, 0x09, 0x09, // walking
    0x18, 0x18, 0x18, 0x19, 0x19, 0x19 // dying
};

#define G(x) (x)*8
#define MINMAX(x,min,max) x=(((x)<(min))?(min):((x)>(max))?(max):(x))
#define SPRITE_COLLISION(x1,y1,x2,y2,sx1,sx2,sy1,sy2) \
          (((x1 - x2) < sx2 || (x2 - x1) < sx1) \
        && ((y1 - y2) < sy2 || (y2 - y1) < sy1))

#define SPRITE_COLLISION_8(x1,y1,x2,y2) \
        SPRITE_COLLISION(x1,y1,x2,y2,8,8,8,8)

#define PLAYER_SPEED 2
#define PLAYER_MIN_X 8
#define PLAYER_MAX_X 96
#define PLAYER_MIN_Y 32
#define PLAYER_MAX_Y 196
#define BONE_MAX_X 248
#define BONE_SPEED 4
#define ENEMY_START_X 248
#define ENEMY_SPEED_X 1

void main(void) {
    pal_all(pal);
    ppu_on_all();

    // initial positions
    dx = PLAYER_MIN_X;
    dy = (PLAYER_MAX_Y + PLAYER_MIN_Y) / 2;
    bx = 0;
    bs = 0;
    es = 0;

    score = 0;
    game_over = 0;

    while (1) {
        ppu_wait_frame();

        i = pad_poll(0);

        // player movement
        if((i & PAD_LEFT)  && dx > PLAYER_MIN_X) dx -= PLAYER_SPEED;
        if((i & PAD_RIGHT) && dx < PLAYER_MAX_X) dx += PLAYER_SPEED;
        if((i & PAD_UP)    && dy > PLAYER_MIN_Y) dy -= PLAYER_SPEED;
        if((i & PAD_DOWN)  && dy < PLAYER_MAX_Y) dy += PLAYER_SPEED;

        // bone movement
        if(bx) bx += BONE_SPEED;
        if(bx > BONE_MAX_X) bx = 0;
        if((i & PAD_A) && bx == 0) {
            bx = dx + G(3);
            by = dy + G(1);
        }

        // enemy movement
        switch (es) {
        case 0:
            // spawn new enemy
            es = 1;
            ex = ENEMY_START_X;

            r = rand8();
            ey = dy + r - (PLAYER_MAX_Y + PLAYER_MIN_Y) / 2;
            MINMAX(ey, PLAYER_MIN_Y+8, PLAYER_MAX_Y-8);
            break;

        case 4:
            es = 0; // will be incremented back to one right after
            // fall through
        case 1: // enemy movement
        case 2:
        case 3:
            ++es;
            ex -= ENEMY_SPEED_X;
            if (ex < PLAYER_MIN_X) {
                es = 0; // enemy disappears
                game_over = 1;
            }
            // collisions
            if (SPRITE_COLLISION(bx, by, ex, ey, 6, 6, 8, 8)) {
                es = 5;
                bx = 0;
                ++score;
            }
            break;

        case 5: // dying
        case 6:
        case 7:
        case 8:
        case 9:
            ++es;
            break;

        default: // dead
            es = 0;
            break;
        }

        // draw sprites
        spr = 0;

        // draw player
        spr = oam_spr(dx+G(2), dy+G(0),  2, 0, spr); // hat
        spr = oam_spr(dx+G(0), dy+G(1), 16, 0, spr); // tail
        spr = oam_spr(dx+G(2), dy+G(1), 18, 1, spr); // head
        spr = oam_spr(dx+G(3), dy+G(1), 19, 1, spr); // nose
        spr = oam_spr(dx+G(0), dy+G(2), 32, 0, spr); // back
        spr = oam_spr(dx+G(1), dy+G(2), 33, 0, spr); // body
        spr = oam_spr(dx+G(2), dy+G(2), 34, 0, spr); // neck

        // draw bone
        if (bx) {
            bs = (bs + 1) & 7;
            spr = oam_spr(bx, by, bone_sprite[bs], 1, spr); // bone
        }

        // draw enemy
        if (ex) {
            spr = oam_spr(ex, ey, enemy_sprite[es], 1, spr); // enemy
        }

        oam_hide_rest(spr);
    };
}
