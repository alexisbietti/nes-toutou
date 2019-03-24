#include "neslib.h"

// Palettes
const unsigned char pal[32]={
    // Background
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

// Projectile animation
const unsigned char bone_sprite[8] = {
    0x06, 0x06, 0x16, 0x16, 0x07, 0x07, 0x16, 0x16
};

// Enemy animations
const unsigned char enemy_sprite[12] = {
    0x00, // hidden
    0x08, 0x08, 0x09, 0x09, // walking
    0x0A, // jumping
    0x18, 0x18, 0x18, 0x19, 0x19, 0x19 // dying
};

#define ENEMY_HIDDEN 0
#define ENEMY_WALKING 1
#define ENEMY_JUMPING 5
#define ENEMY_DYING 6

// frame-by-frame horizontal shift of a jump
const unsigned char enemy_jump[] = {
    0, 0, 3, 6, 9, 11, 13, 15, 16, 17, 18, 17, 16, 15, 13, 11, 9, 6, 3, 0
};

// Background sprites that don't move, for rapid update
static unsigned char list[6*3+1] = {
    // Score (3 digits)
    MSB(NTADR_A(2,2)),LSB(NTADR_A(2,2)),0,
    MSB(NTADR_A(3,2)),LSB(NTADR_A(3,2)),0,
    MSB(NTADR_A(4,2)),LSB(NTADR_A(4,2)),0,

    // Life (3 hearts)
    MSB(NTADR_A(20,2)),LSB(NTADR_A(20,2)),0,
    MSB(NTADR_A(22,2)),LSB(NTADR_A(22,2)),0,
    MSB(NTADR_A(24,2)),LSB(NTADR_A(24,2)),0,

    // that's all folks
    NT_UPD_EOF
};

#define BG_SPRITE(n) list[3*n + 2]

#define G(x) ((x)*8)
#define MINMAX(x,min,max) (x)=(((x)<(min))?(min):((x)>(max))?(max):(x))
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
#define ENEMY_SPEED_Y 1
#define STUN_FRAMES 10
#define ENEMY_MIN_Y (PLAYER_MIN_Y+16)
#define ENEMY_MAX_Y (PLAYER_MAX_Y-8)

#define SPR_EMPTY_HEART 0x19
#define SPR_FULL_HEART 0x18
#define SPR_NONE 0

#define SPR_ZERO 0x30
#define SPR_DIGIT(x) (SPR_ZERO+(x))

#define PLAY_ENEMY_DEAD() sfx_play(0,0)
#define PLAY_PLAYER_HIT() sfx_play(1,1)
#define PLAY_GAME_OVER() sfx_play(2,2)

// Game state
static unsigned char spr; // sprite counter, reset to 0 beginning of each frame
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
static unsigned char score; // what happens when it overflows? I don't care!
static unsigned char life; // dead when it hits 0

void move_enemy(void) {
    ex -= ENEMY_SPEED_X;
    if (ex < PLAYER_MIN_X || SPRITE_COLLISION(dx+8, dy, ex, eyr, G(4), G(2), 8, 8)) {
        es = ENEMY_HIDDEN; // enemy disappears
        ej = 0;
        --life; // lose a life
        if (life) {
          PLAY_PLAYER_HIT();
        }
        stun = STUN_FRAMES; // start stun 'animation'
    } else if (SPRITE_COLLISION(bx, by, ex, eyr, 6, 6, 8, 8)) {
        PLAY_ENEMY_DEAD();
        es = ENEMY_DYING; // enemy start dying animation
        ej = 0;
        bx = 0;
        ++score; // Kill = score
    } else if (ej == 0) {
        // if the enemy is not in jumping state,
        // check if vertical move on 2nd controller
        pp = pad_poll(1);
        if ((pp & PAD_UP) && ey > ENEMY_MIN_Y) {
            ey -= ENEMY_SPEED_Y;
            eyr = ey;
        } else if ((pp & PAD_DOWN) && dy < ENEMY_MAX_Y) {
            ey += ENEMY_SPEED_Y;
            eyr = ey;
        } else {
            // not moving, start a jump, maybe
            r = rand8();
            if (r < 16) {
                ej = 1;
                es = ENEMY_JUMPING;
            }
        }
    }
}

void init_play(void) {
    dx = PLAYER_MIN_X;
    dy = (PLAYER_MAX_Y + PLAYER_MIN_Y) / 2;
    bx = 0;
    bs = 0;
    es = ENEMY_HIDDEN;
    ej = 0;

    score = 0;
    life = 3;
}

void wait_new_play(void) {
    music_stop();
    // wait until player 1 hits start
    do {
        ppu_wait_frame();
        pp = pad_poll(0);
    } while ((pp & PAD_START) == 0);

    // reset game state
    init_play();
    music_play(0);
}

void update_player(void) {
    if (stun) {
        // still stunned, no movement allowed
        --stun;
    } else {
        // horizontal movement
        if     ((pp & PAD_LEFT)  && dx > PLAYER_MIN_X) dx -= PLAYER_SPEED;
        else if((pp & PAD_RIGHT) && dx < PLAYER_MAX_X) dx += PLAYER_SPEED;

        // vertical movement
        if     ((pp & PAD_UP)    && dy > PLAYER_MIN_Y) dy -= PLAYER_SPEED;
        else if((pp & PAD_DOWN)  && dy < PLAYER_MAX_Y) dy += PLAYER_SPEED;
    }
}

void update_bone(void) {
    if(bx) bx += BONE_SPEED;

    // the bone reached right side of the screen
    if(bx > BONE_MAX_X) bx = 0;

    if((pt & PAD_A) && bx == 0) {
        bx = dx + G(3);
        by = dy + 12;
    }
}

void update_enemy(void) {
    switch (es) {
    case ENEMY_HIDDEN:
        // spawn new enemy
        es = ENEMY_WALKING;
        ex = ENEMY_START_X;

        r = rand8();
        ey = dy + r - (PLAYER_MAX_Y + PLAYER_MIN_Y) / 2;
        MINMAX(ey, ENEMY_MIN_Y, ENEMY_MAX_Y);
        eyr = ey;
        break;

    case ENEMY_JUMPING:
        ++ej;
        eyr = ey - enemy_jump[ej];
        if (! enemy_jump[ej]) {
            es = ENEMY_WALKING; // jump is finished
            ej = 0;
        }
        move_enemy();
        break;

    case ENEMY_WALKING: // enemy movement
    case ENEMY_WALKING+1:
    case ENEMY_WALKING+2:
        ++es; // advance one frame in the 'walking' animation
        move_enemy();
        break;

    case ENEMY_WALKING+3:
        es = 1;
        move_enemy();
        break;

    case ENEMY_DYING: // dying
    case ENEMY_DYING+1:
    case ENEMY_DYING+2:
    case ENEMY_DYING+3:
    case ENEMY_DYING+4:
        ++es; // advance one frame in the 'dying' animation
        break;

    default: // dead
        es = ENEMY_HIDDEN;
        break;
    }
}

void draw_player(void) {
    spr = oam_spr(dx+G(2), dy+G(0),  2, 0, spr); // hat
    spr = oam_spr(dx+G(0), dy+G(1), 16, 0, spr); // tail
    spr = oam_spr(dx+G(2), dy+G(1), stun ? 0x03 : 0x12, 1, spr); // head
    spr = oam_spr(dx+G(3), dy+G(1), 19, 1, spr); // nose
    spr = oam_spr(dx+G(0), dy+G(2), 32, 0, spr); // back
    spr = oam_spr(dx+G(1), dy+G(2), 33, 0, spr); // body
    spr = oam_spr(dx+G(2), dy+G(2), 34, 0, spr); // neck
}

void draw_bone(void) {
    if (bx) {
        bs = (bs + 1) & 7;
        spr = oam_spr(bx, by, bone_sprite[bs], 1, spr);
    }
}

void draw_enemy(void) {
    if (ex) {
        spr = oam_spr(ex, eyr, enemy_sprite[es], 1, spr); // enemy

        // if jumping, draw enemy shadow
        if (ej) {
            spr = oam_spr(ex, ey, 0x1A, 1, spr);
        }
    }
}

void update_background(void) {
    // Score
    list[2] = (score < 100) ? SPR_NONE : SPR_DIGIT(score / 100);
    list[5] = (score < 10)  ? SPR_NONE : SPR_DIGIT((score / 10) % 10);
    list[8] = SPR_DIGIT(score % 10);

    // Life
    list[11]=(life<3) ? SPR_EMPTY_HEART : SPR_FULL_HEART;
    list[14]=(life<2) ? SPR_EMPTY_HEART : SPR_FULL_HEART;
    list[17]=(life<1) ? SPR_EMPTY_HEART : SPR_FULL_HEART;
}

void main(void) {
    pal_all(pal);
    set_vram_update(list);
    bank_bg(1);
    ppu_on_all();

    wait_new_play();

    while (1) {
        ppu_wait_frame();

        pt = pad_trigger(0);
        pp = pad_poll(0);

        // update game state
        update_player();
        update_bone();
        update_enemy();

        update_background();

        // draw sprites
        spr = 0;
        draw_player();
        draw_bone();
        draw_enemy();

        // avoid remaining sprites in buffer from showing up on screen
        oam_hide_rest(spr);

        if (life == 0) {
            PLAY_GAME_OVER();
            // enter a new loop
            wait_new_play();
        }
    };
}
