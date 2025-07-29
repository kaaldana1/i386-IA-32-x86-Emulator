#include "program_loader.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define FLAT_MODE_GDT_SIZE 6
// https://wiki.osdev.org/GDT_Tutorial

#define ACCESSED            0x01
#define RW                  0x02
#define DC                  0x04
#define EXECUTABLE          0x08
#define SEG_DESCTYPE(x)     ((x) << 0x04) // Descriptor type, 0 for system, 1 for code/data
#define DESC_DPL0           0x00
#define DESC_DPL1           0x20
#define DESC_DPL2           0x40
#define DESC_DPL3           0x60
#define PRESENT             0x80

#define AVL                 0x01
#define DB_32_BIT           0x04
#define GRAIN_BYTE          0x00
#define GRAIN_4KiB          0x08

#define USER_DATA_RW     (PRESENT | DESC_DPL3 | SEG_DESCTYPE(1) |  RW) // 0xF2
#define USER_CODE_RX     (PRESENT | DESC_DPL3 | SEG_DESCTYPE(1) | EXECUTABLE | RW) //0xFA
#define KERNEL_CODE_RX   (PRESENT | DESC_DPL0 | SEG_DESCTYPE(1) | EXECUTABLE | RW) //0x9A
#define KERNEL_DATA_RW   (PRESENT | DESC_DPL0 | SEG_DESCTYPE(1) | RW)   //0x92

typedef struct __attribute__((packed))  {
    uint16_t limit_low; // byte 0-1
    uint16_t base_low; // byte 2-3
    uint8_t base_mid;  // byte 4
// access byte                       byte 5
//    uint64_t type           
//    uint64_t system         
//    uint64_t dpl            
//    uint64_t present        
    uint8_t access_byte;
// flags and limit                  byte 6
//      G  D/B  L  AVL
    uint8_t flag_limit_high;

    uint64_t base_high;  // byte 7
} gdt_entry;

typedef enum {                  
    NULL_DESCRIPTOR,            
    KERNEL_MODE_CODE_SEG,      
    KERNEL_MODE_DATA_SEG,      
    USER_MODE_CODE_SEG,        
    USER_MODE_DATA_STACK_SEG,  
    TASK_STATE_SEG              
} segment_descriptors;


// global descriptor table
extern gdt_entry global_desc_table[FLAT_MODE_GDT_SIZE];

static int set_gdt_entry(gdt_entry *entry, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    memset(entry, 0, sizeof(gdt_entry));
    entry->base_low = base & 0xFFFF;
    entry->base_mid = (base >> 16) & 0xFF;
    entry->base_high = (base >> 24) & 0xFF;

    entry->limit_low = limit & 0xFFFF;
    entry->flag_limit_high =  ((flags & 0x0F) << 4) | ((limit >> 16) & 0x0F);

    entry->access_byte = access;
    return 0;
}

int create_gdt();

static inline uint32_t get_gdt_limit(const gdt_entry *entry) {
    return (uint32_t)entry->limit_low | (((uint32_t)entry->flag_limit_high & 0x0F ) << 16);
}

static inline uint32_t get_gdt_base(const gdt_entry *entry) {
    return (uint32_t)entry->base_low | ((uint32_t)entry->base_mid << 16) | ((uint32_t)entry->base_high << 24);
}