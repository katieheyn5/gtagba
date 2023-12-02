#include "gta_map.h"
#include "background.h"
#include "cars.h"
#include "text.h"

#include <stdio.h>
#include <stdlib.h>

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
volatile unsigned short* bgtext_palette = (volatile unsigned short*) 0x5000000;
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

    sprites[index].attribute0 = y |             
        (0 << 8) |          
        (0 << 10) |         
        (0 << 12) |         
        (1 << 13) |         
        (shape_bits << 14);

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
    memcpy16_dma((unsigned short*) sprite_palette, (unsigned short*) cars_palette, PALETTE_SIZE);

    memcpy16_dma((unsigned short*) sprite_image_memory, (unsigned short*) cars_data, (cars_width * cars_height) / 2);
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
    //for (int i = 0; i < PALETTE_SIZE; i++) {
        //bg_palette[i] = background_palette[i];
    //}

    //for (int i = 0; i < PALETTE_SIZE; i++) {
        //bgtext_palette[i] = text_palette[i];
    //}

    memcpy16_dma((unsigned short*) bg_palette, (unsigned short*) background_palette, PALETTE_SIZE);
    memcpy16_dma((unsigned short*) char_block(0), (unsigned short*) background_data,
            (background_width * background_height) / 2);

    //memcpy16_dma((unsigned short*) bgtext_palette, (unsigned short*) text_palette, PALETTE_SIZE);
    memcpy16_dma((unsigned short*) char_block(1), (unsigned short*) text_data, (text_width * text_height) / 2);
    
    volatile unsigned short* dest = char_block(0);
    volatile unsigned short* dest1 = char_block(1);
    
    unsigned short* image = (unsigned short*) background_data;
    for (int i = 0; i < ((background_width * background_height) / 2); i++) {
        dest[i] = image[i];
    }


    *bg0_control = 1 |   
        (0 << 2)  |       
        (0 << 6)  |       
        (1 << 7)  |       
        (16 << 8) |       
        (1 << 13) |      
        (0 << 14); 
    
    *bg1_control = 0 |
        (1 << 2) | 
        (0 << 6) |
        (1 << 7) | 
        (24 << 8) | 
        (1 << 13) | 
        (0 << 14);

    dest = screen_block(16);
    for (int i = 0; i < (gta_map_width * gta_map_height); i++) {
        dest[i] = gta_map[i];
    }
    
    dest1 = screen_block(24);
    for (int i = 0; i < 32; i++) {
        dest1[i] = 0;   
    }
}

struct Car {
    struct Sprite* sprite;
    int x, y;
    int frame;
    int counter;
    float move;
    int border;
};

void car_init(struct Car* car, int x, int y, int frame) {
    car->x = x;
    car->y = y;
    car->border = 40;
    car->frame = frame;
    car->move = 0;
    car->sprite = sprite_init(car->x, car->y, SIZE_32_16, 0, 0, car->frame, 0);
}

int policecar_left(struct Car* car) {
    car->move = 1;

    if (car->x < car->border) {
        return 1;
    } else {
        car->x--;
        return 0;
    }
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

int policecar_right(struct Car* policecar, struct Car* currentcar) {
    policecar->move = 1;

    if (currentcar->move == 1)
    {
        if (policecar->x/2 > SCREEN_WIDTH - 49 - policecar->border) {
            return 1;
        }
        else{
            policecar->x++;
            return 0;
        }
    }
    else{
        if (policecar->x/2 > SCREEN_WIDTH - 16 - policecar->border) {
            return 1;
        }
        else{
            policecar->x++;
            return 0;
        }
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
    car->counter = 7;
    sprite_set_offset(car->sprite, car->frame);
}

void car_update(struct Car* car) {
    sprite_position(car->sprite, car->x, car->y);
}

void policecar_update(struct Car* car) {
    sprite_position(car->sprite, car->x/2, car->y);
}

void move_police(struct Car* policecar, struct Car* currentcar){
    int policex = policecar->x/2;
    if (((currentcar->x) > policex)){
        policecar_right(policecar, currentcar);
    }
    else if (((currentcar->x) < policex)){
        policecar_left(policecar);
    }
    else{
        policecar_right(policecar, currentcar);
    }
    
    int currenty = currentcar->y;
    if (((currentcar->y) < 73) & ((policecar->y) > currenty)){
        car_up(policecar);  
    }
    else if (((currentcar->y) > 73) & ((policecar->y) < currenty)){
        car_down(policecar);
    }
}

void subtract(int* num_lives);
void reset(int* num_lives);

void collision(struct Car* policecar, struct Car* currentcar, int* num_lives){
    policecar->x = 45;
    currentcar->x = 100;
    policecar->y = 90;
    currentcar->y = 90;
    subtract(num_lives);
    if (num_lives = 0) {
        reset(num_lives);
    }
}

void check(struct Car* policecar, struct Car* currentcar, int* lives){
    int policex = policecar->x/2;
    int policey = policecar->y;
    int currentx = currentcar->x;
    int currenty = currentcar->y;

    if(((policex <= currentx) & (currentx <= (policex + 32))) & ((currenty-16)==policey)){
        collision(policecar, currentcar, lives);
    }
    else if(((policex <= currentx) & (currentx <= (policex + 32))) & (currenty==(policey-16))){
        collision(policecar, currentcar, lives);
    }
    else if(((policex+32) == currentx) & (((currenty-16)<=policey) & (policey<=currenty))){
        collision(policecar, currentcar, lives);
    }
    
}

void set_text(char* str, int row, int col) {                    
    int index = row * 32 + col;

    int missing = 32; 

    volatile unsigned short* ptr = screen_block(24);

    while (*str) {
        ptr[index] = *str - missing;

        index++;
        str++;
    }   
}

void delay(unsigned int amount) {
    for (int i = 0; i < amount * 10; i++);
}

int main() {
    *display_control = MODE0 | BG0_ENABLE | BG1_ENABLE | SPRITE_ENABLE | SPRITE_MAP_1D;    

    setup_background();
    
    //char text [8] = "Lives: ";
    //set_text(text, 0,0);    
    
    int lives = 3;
    char slives[12];
   
    //char slives2 [8] = "3"; 
    //char livestext [2] = *slives;
    sprintf(slives, "Lives: %d", lives);
    set_text(slives, 0,0);

    setup_sprite_image();
    sprite_clear();

    struct Car redcar;
    car_init(&redcar, 120, 80, 0);
    struct Car greencar;
    car_init(&greencar, 85, 25, 16);
    struct Car policecar;
    car_init(&policecar, 10, 90, 32);
    struct Car *currentcar = &redcar;

    int xscroll = 0;

    while (1) {
        car_update(&redcar);
        car_update(&greencar);
        policecar_update(&policecar);

        if(button_pressed(BUTTON_A)){
            currentcar = &greencar;
            currentcar->frame = 16;
        }
        else if(button_pressed(BUTTON_B)){
            currentcar = &redcar;
            currentcar->frame = 0;
        }        
        if (button_pressed(BUTTON_RIGHT)) {
            if (car_right(currentcar)) {
                xscroll++;
            }
        } else if (button_pressed(BUTTON_LEFT)) {
            if (car_left(currentcar)) {
                xscroll--;
            }
        } else if (button_pressed(BUTTON_UP)) {
            car_up(currentcar);
        } else if (button_pressed(BUTTON_DOWN)) { 
            car_down(currentcar);
        } else {
            car_stop(currentcar);
        }

        move_police(&policecar, currentcar); 
        check(&policecar, currentcar, &lives);        

        wait_vblank();
        *bg0_x_scroll = xscroll;
        sprite_update_all();

        sprintf(slives, "Lives: %d", lives);
        set_text(slives, 0,0);

        delay(100);
    }
}
