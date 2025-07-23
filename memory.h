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

#define FLAT_MODE_GDT_SIZE 6

typedef struct __attribute__((packed))  {
    uint16_t limit_low; // byte 0-1
    uint16_t base_low; // byte 2-3
    uint8_t base_mid;  // byte 4
// access byte                       byte 5
//    uint64_t type           : 3;
//    uint64_t system         : 1;
//    uint64_t dpl            : 3;
//    uint64_t present        : 1;
    uint8_t access_byte;
// flags and limit                  byte 6
    uint8_t flag_limit_high;

    uint64_t base_high;  // byte 7
} gdt_entry;

typedef enum {                  
    NULL_DESCRIPTOR,            
    KERNEL_MODE_CODE_SEG,       // CS
    KERNEL_MODE_DATA_SEG,       // DS 
    USER_MODE_CODE_SEG,         // CS   
    USER_MODE_DATA_SEG,         // DS
    TASK_STATE_SEG              
} segment_descriptors;


// global descriptor table
extern gdt_entry global_desc_table[FLAT_MODE_GDT_SIZE];

extern uint8_t ram_16kb[RAM_SIZE];
extern uint32_t address_bus;
extern uint32_t data_bus;

int initialize_ram(Program *p);
int set_gdt_entry(gdt_entry *entry, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags);
// either writes to ram or console port
int memory_write_byte(uint8_t value, uint32_t address); 
int memory_write_word(uint16_t value, uint32_t address); 
int memory_write_dword(uint32_t value, uint32_t address); 

// read from keyboard input port
int memory_read_byte (uint8_t *value, uint32_t address);
int memory_read_dword (uint32_t *value, uint32_t address);

#endif