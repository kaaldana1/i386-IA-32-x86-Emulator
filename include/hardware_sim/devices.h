#ifndef DEVICES_H
#define DEVICES_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "hardware_sim/memmap.h"


typedef struct RAMDev RAMDev;
typedef struct CONSOLEDev CONSOLEDev;
typedef struct VGADev VGADev;
typedef struct coord coord;

RAMDev *init_ram(void);
CONSOLEDev *init_console(void);

int console_read_stub(void *device, uint32_t *value, uint32_t address, size_t width);
int console_write(void *device, uint32_t data, uint32_t addr, size_t width);

int ram_read(RAMDev *r, uint32_t *value, uint32_t address, size_t width);
int ram_write(RAMDev *r, uint32_t value, uint32_t address, size_t width);

VGADev *init_vga(void);
int vram_write(void *device, uint32_t data, uint32_t addr, size_t width);
int vram_read_stub(void *device, uint32_t *value, uint32_t address, size_t width);

coord find_cell(uint32_t cell_index);

#endif
