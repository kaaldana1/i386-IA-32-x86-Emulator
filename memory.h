#ifndef MEMORY_H 
#define MEMORY_H 

#include "program_loader.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define RAM_SIZE 16384 // 16 KBs total
#define HIGHEST_RAM_ADDRESS 0x3FFF
#define LOWEST_RAM_ADDRESS 0x0000
#define CONSOLE_PORT 0x4000
#define KEYBOARD_PORT 0x400F
#define CPU_BUS_SIZE 4 // 4 bytes, 32 bits

typedef struct
{
    // 14 address bits
    uint16_t text_base, data_base,
        stack_base, heap_base;
    size_t text_size, data_size;
} Ram_map;

extern Ram_map ram_map;
extern uint8_t ram_16kb[RAM_SIZE];
extern uint32_t address_bus;
extern uint32_t data_bus;

int initialize_ram(Program *p);

// either writes to ram or console port
int memory_write_byte(uint8_t value, uint32_t address); 
int memory_write_word(uint16_t value, uint32_t address); 
int memory_write_dword(uint32_t value, uint32_t address); 

// read from keyboard input port
int memory_read_byte( uint8_t *value, uint32_t address);

#endif