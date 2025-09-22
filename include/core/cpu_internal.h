#ifndef CPU_INTERNAL_H
#define CPU_INTERNAL_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "hardware_sim/bus.h"
#include "core/structs/FlagPolicy.h" 

#define SegmentRegister_AMOUNT 6
#define GeneralPurposeRegister_AMOUNT 9
#define GDT_DESC_SIZE 8 

typedef union
{
    uint32_t dword;
    uint16_t word[2];
    uint8_t byte[4];
} GeneralPurposeRegister; // general purpose register


typedef struct 
{
    uint16_t selector;

    /* secret cache */
    uint32_t base, limit;
    uint16_t flag;
} SegmentRegister; // segment register

typedef struct {
    uint32_t base;
    uint32_t size;
} GDTR;

struct CPU {
    GeneralPurposeRegister gen_purpose_registers[GeneralPurposeRegister_AMOUNT]; //indexed by GeneralPurposeRegisterType 
    SegmentRegister segment_registers[SegmentRegister_AMOUNT]; //indexed by GeneralPurposeRegisterType 
    uint16_t status_register; 
    GDTR gdtr;
    bool halt;
}; 
#endif
