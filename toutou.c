#include "neslib.h"


static unsigned char spr;
static unsigned char pt, pp; // pad triggers and pad state
static unsigned char dx, dy; // player position
static unsigned char stun; // number of frames the player is stunned
static unsigned char bx, by; // bone position
static unsigned char bs; // bone sprite, must be < 4
static unsigned char ex, ey; // enemy position
static unsigned char eyr; // enemy real y position
static unsigned char ej; // enemy jump state
static unsigned char es; // enemy sprite 0 = no enemy
static unsigned char r; // random number
static unsigned char score;
static unsigned char life;

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

const unsigned char enemy_sprite[12] = {
    0x00, // hidden
    0x08, 0x08, 0x09, 0x09, // walking
    0x0A, // jumping
    0x18, 0x18, 0x18, 0x19, 0x19, 0x19 // dying
};

const unsigned char enemy_jump[] = {
    0, 0, 3, 6, 9, 11, 13, 15, 16, 17, 18, 17, 16, 15, 13, 11, 9, 6, 3, 0
};

static unsigned char list[6*3+1] = {
    MSB(NTADR_A(2,2)),LSB(NTADR_A(2,2)),0, //score
    MSB(NTADR_A(3,2)),LSB(NTADR_A(3,2)),0,
    MSB(NTADR_A(4,2)),LSB(NTADR_A(4,2)),0,
    MSB(NTADR_A(20,2)),LSB(NTADR_A(20,2)),0, //life
    MSB(NTADR_A(22,2)),LSB(NTADR_A(22,2)),0,
    MSB(NTADR_A(24,2)),LSB(NTADR_A(24,2)),0,
    NT_UPD_EOF
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
#define STUN_FRAMES 10

void move_enemy(void) {
    ex -= ENEMY_SPEED_X;
    if (ex < PLAYER_MIN_X || SPRITE_COLLISION(dx+8, dy, ex, eyr, G(4), G(2), 8, 8)) {
        es = 0; // enemy disappears
        ej = 0;
        --life; // lose a life
        stun = STUN_FRAMES;
    } else if (SPRITE_COLLISION(bx, by, ex, eyr, 6, 6, 8, 8)) {
        es = 6;
        ej = 0;
        bx = 0;
        ++score;
    }

    // start a jump
    if (ej == 0) {
        r = rand8();
        if (r < 16) {
            ej = 1;
            es = 5;
        }
    }

    // collisions
}

void init_play(void) {
    dx = PLAYER_MIN_X;
    dy = (PLAYER_MAX_Y + PLAYER_MIN_Y) / 2;
    bx = 0;
    bs = 0;
    es = 0;
    ej = 0;

    score = 0;
    life = 3;
}

void wait_new_play(void) {
    while (1) {
        ppu_wait_frame();
        pp = pad_poll(0);

        if (pp & PAD_START) {
            init_play();
            return;
        }
    }
}

void main(void) {
    pal_all(pal);
    set_vram_update(list);
    bank_bg(1);
    ppu_on_all();

    init_play();

    while (1) {
        ppu_wait_frame();

        pt = pad_trigger(0);
        pp = pad_poll(0);

        // player movement
        if (stun == 0) {
            if     ((pp & PAD_LEFT)  && dx > PLAYER_MIN_X) dx -= PLAYER_SPEED;
            else if((pp & PAD_RIGHT) && dx < PLAYER_MAX_X) dx += PLAYER_SPEED;
            if     ((pp & PAD_UP)    && dy > PLAYER_MIN_Y) dy -= PLAYER_SPEED;
            else if((pp & PAD_DOWN)  && dy < PLAYER_MAX_Y) dy += PLAYER_SPEED;
        } else {
            --stun;
        }

        // bone movement
        if(bx) bx += BONE_SPEED;
        if(bx > BONE_MAX_X) bx = 0;

        if((pt & PAD_A) && bx == 0) {
            bx = dx + G(3);
            by = dy + 12;
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
            eyr = ey;
            break;

        case 5: // jumping
            ++ej;
            eyr = ey - enemy_jump[ej];
            if (! enemy_jump[ej]) {
                es = 1; // jump is finished
                ej = 0;
            }
            move_enemy();
            break;

        case 1: // enemy movement
        case 2:
        case 3:
            ++es;
            move_enemy();
            break;

        case 4:
            es = 1;
            move_enemy();
            break;

        case 6: // dying
        case 7:
        case 8:
        case 9:
        case 10:
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
        spr = oam_spr(dx+G(2), dy+G(1), stun ? 0x03 : 0x12, 1, spr); // head
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
            spr = oam_spr(ex, eyr, enemy_sprite[es], 1, spr); // enemy

            // draw enemy shadow
            if (ej) {
                spr = oam_spr(ex, ey, 0x1A, 1, spr);
            }
        }

        // score
        list[2]=(score<100)?0x00:(0x30+score/100);
        list[5]=(score<10)?0x00:(0x30+score/10%10);
        list[8]=0x30+score%10;

        // life
        list[11]=(life<3)?0x19:0x18;
        list[14]=(life<2)?0x19:0x18;
        list[17]=(life<1)?0x19:0x18;

        oam_hide_rest(spr);

        if (life == 0) {
            wait_new_play();
        }
    };
}
