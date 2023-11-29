#include "gta_map.h"
#include "background.h"
#include "redcar.h"
#include "greencar.h"

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

#define MODE0 0x00
#define MODE1 0x01
#define MODE2 0x02

#define BG0_ENABLE 0x100
#define BG1_ENABLE 0x200
#define BG2_ENABLE 0x400
#define BG3_ENABLE 0x800

#define SPRITE_MAP_2D 0x0
#define SPRITE_MAP_1D 0x40
#define SPRITE_ENABLE 0x1000
#define NUM_SPRITES 128

volatile unsigned short* bg0_control = (volatile unsigned short*) 0x4000008;
volatile unsigned short* bg1_control = (volatile unsigned short*) 0x400000a;
volatile unsigned short* bg2_control = (volatile unsigned short*) 0x400000c;
volatile unsigned short* bg3_control = (volatile unsigned short*) 0x400000e;

#define PALETTE_SIZE 256

volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;
volatile unsigned short* bg_palette = (volatile unsigned short*) 0x5000000;
volatile unsigned short* buttons = (volatile unsigned short*) 0x04000130;

volatile short* bg0_x_scroll = (unsigned short*) 0x4000010;
volatile short* bg0_y_scroll = (unsigned short*) 0x4000012;
volatile short* bg1_x_scroll = (unsigned short*) 0x4000014;
volatile short* bg1_y_scroll = (unsigned short*) 0x4000016;
volatile short* bg2_x_scroll = (unsigned short*) 0x4000018;
volatile short* bg2_y_scroll = (unsigned short*) 0x400001a;
volatile short* bg3_x_scroll = (unsigned short*) 0x400001c;
volatile short* bg3_y_scroll = (unsigned short*) 0x400001e;

volatile unsigned short* sprite_attribute_memory = (volatile unsigned short*) 0x7000000;
volatile unsigned short* sprite_image_memory = (volatile unsigned short*) 0x6010000;
volatile unsigned short* sprite_palette = (volatile unsigned short*) 0x5000200;

#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)

volatile unsigned short* scanline_counter = (volatile unsigned short*) 0x4000006;

#define DMA_ENABLE 0x80000000
#define DMA_16 0x00000000
#define DMA_32 0x04000000

volatile unsigned int* dma_source = (volatile unsigned int*) 0x40000D4;
volatile unsigned int* dma_destination = (volatile unsigned int*) 0x40000D8;
volatile unsigned int* dma_count = (volatile unsigned int*) 0x40000DC;

void memcpy16_dma(unsigned short* dest, unsigned short* source, int amount) {
    *dma_source = (unsigned int) source;
    *dma_destination = (unsigned int) dest;
    *dma_count = amount | DMA_16 | DMA_ENABLE;
}

void wait_vblank() {volatile unsigned short* sprite_palette = (volatile unsigned short*) 0x5000200;
    while (*scanline_counter < 160) { }
}
struct Sprite {
    unsigned short attribute0;
    unsigned short attribute1;
    unsigned short attribute2;
    unsigned short attribute3;
};

struct Sprite sprites[NUM_SPRITES];
int next_sprite_index = 0;

enum SpriteSize {
    SIZE_8_8,
    SIZE_16_16,
    SIZE_32_32,
    SIZE_64_64,
    SIZE_16_8,
    SIZE_32_8,
    SIZE_32_16,
    SIZE_64_32,
    SIZE_8_16,
    SIZE_8_32,
    SIZE_16_32,
    SIZE_32_64
};

struct Sprite* sprite_init(int x, int y, enum SpriteSize size,
    int horizontal_flip, int vertical_flip, int tile_index, int priority) {
    int index = next_sprite_index++;
    int size_bits, shape_bits;
    switch (size) {
        case SIZE_8_8:   size_bits = 0; shape_bits = 0; break;
        case SIZE_16_16: size_bits = 1; shape_bits = 0; break;
        case SIZE_32_32: size_bits = 2; shape_bits = 0; break;
        case SIZE_64_64: size_bits = 3; shape_bits = 0; break;
        case SIZE_16_8:  size_bits = 0; shape_bits = 1; break;
        case SIZE_32_8:  size_bits = 1; shape_bits = 1; break;
        case SIZE_32_16: size_bits = 2; shape_bits = 1; break;
        case SIZE_64_32: size_bits = 3; shape_bits = 1; break;
        case SIZE_8_16:  size_bits = 0; shape_bits = 2; break;
        case SIZE_8_32:  size_bits = 1; shape_bits = 2; break;
        case SIZE_16_32: size_bits = 2; shape_bits = 2; break;
        case SIZE_32_64: size_bits = 3; shape_bits = 2; break;
    }

    int h = horizontal_flip ? 1 : 0;
    int v = vertical_flip ? 1 : 0;

    /* set up the first attribute */
    sprites[index].attribute0 = y |             /* y coordinate */
        (0 << 8) |          /* rendering mode */
        (0 << 10) |         /* gfx mode */
        (0 << 12) |         /* mosaic */
        (1 << 13) |         /* color mode, 0:16, 1:256 */
        (shape_bits << 15); /* shape */

    sprites[index].attribute1 = x |             
        (0 << 9) |         
        (h << 12) |         
        (v << 13) |         
        (size_bits << 14); 

    sprites[index].attribute2 = tile_index |   
        (priority << 10) | 
        (0 << 12);         

    return &sprites[index];
}

void sprite_update_all() {
    memcpy16_dma((unsigned short*) sprite_attribute_memory, (unsigned short*) sprites, NUM_SPRITES * 4);
}

void sprite_clear() {
    next_sprite_index = 0;

    for(int i = 0; i < NUM_SPRITES; i++) {
        sprites[i].attribute0 = SCREEN_HEIGHT;
        sprites[i].attribute1 = SCREEN_WIDTH;
    }
}

void sprite_position(struct Sprite* sprite, int x, int y) {
    sprite->attribute0 &= 0xff00;
    sprite->attribute0 |= (y & 0xff);
    sprite->attribute1 &= 0xfe00;
    sprite->attribute1 |= (x & 0x1ff);
}

void sprite_move(struct Sprite* sprite, int dx, int dy) {
    int y = sprite->attribute0 & 0xff;
    int x = sprite->attribute1 & 0x1ff;
    sprite_position(sprite, x + dx, y + dy);
}

void sprite_set_offset(struct Sprite* sprite, int offset) {
    sprite->attribute2 &= 0xfc00;
    sprite->attribute2 |= (offset & 0x03ff);
}

void setup_sprite_image() {
    memcpy16_dma((unsigned short*) sprite_palette, (unsigned short*) redcar_palette, PALETTE_SIZE);

    memcpy16_dma((unsigned short*) sprite_image_memory, (unsigned short*) redcar_data, (redcar_width * redcar_height) / 2);
}

unsigned char button_pressed(unsigned short button) {
    unsigned short pressed = *buttons & button;
    if (pressed == 0) {
        return 1;
    } else {
        return 0;
    }
}

volatile unsigned short* char_block(unsigned long block) {
    return (volatile unsigned short*) (0x6000000 + (block * 0x4000));
}

volatile unsigned short* screen_block(unsigned long block) {
    return (volatile unsigned short*) (0x6000000 + (block * 0x800));
}

void setup_background() {
    for (int i = 0; i < PALETTE_SIZE; i++) {
        bg_palette[i] = background_palette[i];
    }

    volatile unsigned short* dest = char_block(0);
    unsigned short* image = (unsigned short*) background_data;
    for (int i = 0; i < ((background_width * background_height) / 2); i++) {
        dest[i] = image[i];
    }

    *bg0_control = 0 |   
        (0 << 2)  |       
        (0 << 6)  |       
        (1 << 7)  |       
        (16 << 8) |       
        (1 << 13) |      
        (0 << 14); 

    dest = screen_block(16);
    for (int i = 0; i < (gta_map_width * gta_map_height); i++) {
        dest[i] = gta_map[i];
    }
}

struct Car {
    struct Sprite* sprite;
    int x, y;
    int frame;
    int counter;
    int move;
    int border;
};

void car_init(struct Car* car, int x, int y) {
    car->x = x;
    car->y = y;
    car->border = 40;
    car->frame = 0;
    car->move = 0;
    car->sprite = sprite_init(car->x, car->y, SIZE_16_32, 0, 0, car->frame, 0);
}

int car_left(struct Car* car) {
    car->move = 1;

    if (car->x < car->border) {
        return 1;
    } else {
        car->x--;
        return 0;
    }
}

int car_right(struct Car* car) {
    car->move = 1;

    if (car->x > (SCREEN_WIDTH - 16 - car->border)) {
        return 1;
    } else {
        car->x++;
        return 0;
    }
}

int car_up(struct Car* car) {
   car->move = 1; 

    if (car->y < ((car->border) - 15)) {
        return 1;
    } else {
        car->y--;
        return 0;
    } 
}

int car_down(struct Car* car) {
    car->move = 1;

    if (car->y > (SCREEN_HEIGHT - 16 - car->border)) {
        return 1;
    } else {
        car->y++;
        return 0;
    }
}

void car_stop(struct Car* car) {
    car->move = 0;
    car->frame = 0;
    car->counter = 7;
    sprite_set_offset(car->sprite, car->frame);
}

void car_update(struct Car* car) {
    if (car->move) {
         car->frame = car->frame + 16;
         if (car->frame > 16) {
             car->frame = 0;
             sprite_set_offset(car->sprite, car->frame);
        }
    }
    sprite_position(car->sprite, car->x, car->y);
}

void delay(unsigned int amount) {
    for (int i = 0; i < amount * 10; i++);
}

int main() {
    *display_control = MODE0 | BG0_ENABLE | SPRITE_ENABLE | SPRITE_MAP_1D;    

    setup_background();
    
    setup_sprite_image();
    sprite_clear();

    struct Car redcar;
    car_init(&redcar, 100, 100);
    struct Car greencar;
    car_init(&greencar, 45, 120);

    int xscroll = 0;

    while (1) {
        car_update(&redcar);
        car_update(&greencar);

        if (button_pressed(BUTTON_RIGHT)) {
            if (car_right(&redcar)) {
                xscroll++;
            }
        } else if (button_pressed(BUTTON_LEFT)) {
            if (car_left(&redcar)) {
                xscroll--;
            }
        } else if (button_pressed(BUTTON_UP)) {
            car_up(&redcar);
        } else if (button_pressed(BUTTON_DOWN)) { 
            car_down(&redcar);
        } else {
            car_stop(&redcar);
        }
        
        wait_vblank();
        *bg0_x_scroll = xscroll;
        sprite_update_all();
        
        delay(100);
    }
}
