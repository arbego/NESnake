#include <conio.h>
#include "neslib.h"
#include "level.h"


//LEVEL INFO
#define SCREEN_WIDTH    256
#define SCREEN_HEIGHT   240
#define ATTRIBUTE_TABLE_A 0x23C0

const unsigned char spritesPal[16] = {
    0x0F, 0x11, 0x21, 0x31,
    0x0F, 0x12, 0x22, 0x32,
    0x0F, 0x13, 0x23, 0x33,
    0x0F, 0x14, 0x24, 0x34
};

const unsigned char bgPal[16] = {
    0x0F, 0x25, 0x24, 0x34,
    0x0F, 0x2A, 0x22, 0x32,
    0x0F, 0x13, 0x23, 0x33,
    0x0F, 0x14, 0x24, 0x34
};

const unsigned char levelBoundaries[4] = {
    5 * 8, 5 * 8, 26 * 8, 24 * 8
};

static unsigned char levelNamRef[1024];



//SNAKE INFO
#define SNAKE_PALETTE       0
#define SNAKE_SPRITE_SIZE   8
#define SNAKE_SPRITE        0x42
#define SNAKE_PALETTE       0
#define MAX_SNAKE_SIZE      50
#define SnakeHead           snakeCoords[0]

static unsigned char snakeCoords[MAX_SNAKE_SIZE][2];
static unsigned char snakeSpeed = 8;
static unsigned char snakeSize;
static unsigned char x, y;



// PILLS INFO
#define PILL_SPRITE_SIZE    8
#define PILL_SPRITE         0x40
#define PILL_PALETTE        0
#define MAX_PILLS           5

static unsigned char pillsCreated = 0;
static unsigned char pillsLive = 0;
static unsigned char pillsPositions[MAX_PILLS][2];

static unsigned char pad;
static unsigned char gameover;
static unsigned char i, j, k, tmp, tmp2;
static unsigned char oamBuffer;
static unsigned char vFrameCount;

void reset()
{
    vFrameCount = 0;
    snakeSize = 1;
    SnakeHead[0] = SCREEN_WIDTH >> 1;
    SnakeHead[1] = SCREEN_HEIGHT >> 1;
    x = snakeSpeed;
    y = 0;
}

void main(void)
{
    pal_spr(spritesPal);
    pal_bg(bgPal);

    //TODO: remove copy by packing level_nam contents
    memcpy(levelNamRef, level_nam, 1024);
    
    vram_adr(NAMETABLE_A);
    vram_write(levelNamRef, 1024);

    oam_clear();
    ppu_on_all();

    reset();

    while(1)
    {
        ppu_wait_frame();
        gameover = 0;
        vFrameCount++;

        pad = pad_trigger(0);
        if (pad & PAD_LEFT) {
            gameover = snakeSize > 1 && (x == snakeSpeed);
            x = -snakeSpeed;
            y = 0;
        } else if (pad & PAD_RIGHT) {
            gameover = snakeSize > 1 && (x == 0xFF);
            x = snakeSpeed;
            y = 0;
        } else if (pad & PAD_UP) {
            gameover = snakeSize > 1 && (y == snakeSpeed);
            x = 0;
            y = -snakeSpeed;
        } else if (pad & PAD_DOWN) {
            gameover = snakeSize > 1 && (y == 0xFF);
            x = 0;
            y = snakeSpeed;
        }

        
        tmp2 = 0;
        if (pillsLive == 0) {
            pillsCreated = 0;
            for (i = 0; i < MAX_PILLS; ++i) {
                j = rand8() % (levelBoundaries[2] - levelBoundaries[0]);
                k = rand8() % (levelBoundaries[3] - levelBoundaries[1]);
                pillsPositions[i][0] = levelBoundaries[0] + j;
                pillsPositions[i][1] = levelBoundaries[1] + k;
                ++pillsCreated;
                ++pillsLive;
            }
        } else {
            for (i = 0; i < MAX_PILLS; i++) {  
                if (SnakeHead[0] > (pillsPositions[i][0] - SNAKE_SPRITE_SIZE)
                    && SnakeHead[0] < (pillsPositions[i][0] + SNAKE_SPRITE_SIZE)
                    && SnakeHead[1] > (pillsPositions[i][1] - SNAKE_SPRITE_SIZE)
                    && SnakeHead[1] < (pillsPositions[i][1] + SNAKE_SPRITE_SIZE)
                    ) {
                    pillsPositions[i][0] = -1;
                    pillsPositions[i][1] = -1;
                    --pillsLive;

                    tmp2 = 1;
                    //Grow snake code
                    tmp = snakeSize;
                    ++snakeSize;
                    snakeSize = (snakeSize > MAX_SNAKE_SIZE) ? MAX_SNAKE_SIZE : snakeSize;
                    if (snakeSize != tmp) {
                        snakeCoords[snakeSize - 1][0] = SnakeHead[0];
                        snakeCoords[snakeSize - 1][1] = SnakeHead[1];
                        SnakeHead[0] += x;
                        SnakeHead[1] += y;    
                    }
                }
            }
        }

        //Move snake
        if (vFrameCount > snakeSpeed && tmp2 == 0) {
            vFrameCount = 0;
            for (i = snakeSize - 1; i > 0; --i) {
                snakeCoords[i][0] = snakeCoords[i-1][0];
                snakeCoords[i][1] = snakeCoords[i-1][1];
            }
            SnakeHead[0] += x;
            SnakeHead[1] += y;
        }


        //Check gameover
        gameover = gameover || (SnakeHead[0] <= levelBoundaries[0]);
        gameover = gameover || (SnakeHead[0] >= levelBoundaries[2]);
        gameover = gameover || (SnakeHead[1] <= levelBoundaries[1]);
        gameover = gameover || (SnakeHead[1] >= levelBoundaries[3]);
        if (gameover) {
            reset();
        }


        oam_clear();
        oamBuffer = 0;

        //Draw snake
        for (i = 0; i < snakeSize; ++i) {
            oamBuffer = oam_spr(snakeCoords[i][0], snakeCoords[i][1], SNAKE_SPRITE, SNAKE_PALETTE, oamBuffer);    
        }


        //Draw pills
        for (i = 0; i < pillsCreated; ++i) {
            if (pillsPositions[i][0] != -1 && pillsPositions[i][1] != -1) {
                oamBuffer = oam_spr(pillsPositions[i][0], pillsPositions[i][1], PILL_SPRITE, PILL_PALETTE, oamBuffer);
            }
        }
        

        

        //Check game over
        //READ VRAM AND CHECK IF THERE IS A SPRITE THERE


        // ppu_wait_frame();//wait for next TV frame
        // oam_spr(x, y, POINTER_SPRITE, palette & 3, 0);

        // //Trigger for palette change
        // pad = pad_trigger(0);

        // //Converting if on & formula
        // // if (condition) {
        // //     x += t;
        // // }
        // // Can be expressed as:
        // // x += t & (0x0 - condition)
        // palette += 1 & (0x0 - ((pad & PAD_A) && 1));
        // palette -= 1 & (0x0 - ((pad & PAD_B) && 1));


        // //Poll for arrow movement
        // pad = pad_poll(0);
        // x -= POINTER_SPEED & (0x0 - (pad & PAD_LEFT && x >= 2));
        // x += POINTER_SPEED & (0x0 - (pad & PAD_RIGHT && x < (SCREEN_WIDTH - POINTER_SIZE)));
        // y -= POINTER_SPEED & (0x0 - (pad & PAD_UP && y >= 2));
        // y += POINTER_SPEED & (0x0 - (pad & PAD_DOWN && y < (SCREEN_HEIGHT - POINTER_SIZE)));
    }
}