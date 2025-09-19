#ifndef DEVICES_H
#define DEVICES_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define RAM_SIZE 16384 // 16 KBs total
#define HIGHEST_RAM_ADDRESS 0x3FFF
#define LOWEST_RAM_ADDRESS 0x0000


typedef struct RAMDev RAMDev;
typedef struct CONSOLEDev CONSOLEDev;

RAMDev *init_ram(void);
CONSOLEDev *init_console(void);

int console_read_stub(void *device, uint32_t *value, uint32_t address, size_t width);
int console_write(void *device, uint32_t data, uint32_t addr, size_t width);

int ram_read(RAMDev *r, uint32_t *value, uint32_t address, size_t width);
int ram_write(RAMDev *r, uint32_t value, uint32_t address, size_t width);

#endif
