#include "program_loader.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define RAM_SIZE 16384 // 16 KBs total
#define HIGHEST_ADDRESS 0x3FF 
#define LOWEST_ADDRESS 0x000


typedef struct {
// 14 address bits
    uint16_t text_base, data_base, 
        stack_base, heap_base;
    size_t text_size, data_size;
} Ram_structure;

static Ram_structure ram_struct;
uint8_t ram_16kb[RAM_SIZE];

int initialize_ram(Program *p) { 
    memset(ram_16kb, 0x00, sizeof(ram_16kb));
    memset(&ram_struct, 0, sizeof(Ram_structure));

    // Load text portion of ram with the hex instructions
    ram_struct.text_base = LOWEST_ADDRESS;
    memcpy(ram_16kb + ram_struct.text_base, p->program, p->program_length);
    ram_struct.text_size = p->program_length; // Max index would be prog_len-1


    ram_struct.stack_base = HIGHEST_ADDRESS;
                                                    // to also include global/static data
    ram_struct.heap_base = (uint16_t)ram_struct.text_size;
    return 1;
}

