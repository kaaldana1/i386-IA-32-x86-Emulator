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

int console_read_stub(void *device, uint32_t *value, uint32_t address);
int console_write(void *device, uint32_t data, uint32_t addr);
int ram_read_dword(void *ram_dev, uint32_t *value, uint32_t address);
int ram_write_dword(void *ram_dev, uint32_t value, uint32_t address); 

#endif
