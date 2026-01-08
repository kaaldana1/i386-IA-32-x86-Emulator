#include "devices.h"

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
};

typedef struct coord
{
    uint32_t x;
    uint32_t y;
} coord;

coord find_cell(uint32_t cell_index);
