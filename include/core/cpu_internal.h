#ifndef CPU_INTERNAL_H
#define CPU_INTERNAL_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "hardware_sim/bus.h"

#define SR_AMOUNT 6
#define GPR_AMOUNT 9
#define GDT_DESC_SIZE 8 

typedef union
{
    uint32_t dword;
    uint16_t word[2];
    uint8_t byte[4];
} GPR; // general purpose register


typedef struct 
{
    uint16_t selector;

    /* secret cache */
    uint32_t base, limit;
    uint16_t flag;
} SR; // segment register

typedef struct {
    uint32_t base;
    uint32_t size;
} GDTR;

struct CPU {
    GPR gen_purpose_registers[GPR_AMOUNT]; //indexed by gpr_type 
    SR segment_registers[SR_AMOUNT]; //indexed by gpr_type 
    GDTR gdtr;
    bool halt;
}; 
#endif
