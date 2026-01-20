#include "devices.h"


typedef int IRQNumber;

typedef int(*keyboard_enqueue)(uint8_t *buffer, uint8_t data, size_t *keystrokes_in_queue, size_t size);
typedef int (*keyboard_dequeue)(uint8_t *buffer, uint8_t *data, size_t *keystrokes_in_queue, size_t size);

typedef void(*interrupt_line)(IRQNumber irq_num);

typedef struct Interrupter Interrupter;
struct Interrupter {
    interrupt_line interrupt_line;
    IRQNumber irq_num;
};

struct RAMDev 
{
    uint8_t ram_16kb[RAM_SIZE];
};

struct CONSOLEDev
{
    uint8_t data[4];
};

struct VGADev
{
    uint16_t vram[SCREEN_HEIGHT][SCREEN_WIDTH];
    uint32_t dirty[SCREEN_WIDTH * SCREEN_HEIGHT];
    bool dirty_map[SCREEN_WIDTH * SCREEN_HEIGHT];
    int counter_vram_changes;
    int refresh_rate; /*based on time*/
    Interrupter interrupter;
};

struct KeyboardDev
{
    uint8_t read;
    uint8_t keyboard_buffer[10];
    size_t keystrokes_in_queue;
    bool status;
    Interrupter interrupter;
    keyboard_dequeue dequeue;
    keyboard_enqueue enqueue;
};

typedef struct coord
{
    uint32_t x;
    uint32_t y;
} coord;

coord find_cell(uint32_t cell_index);