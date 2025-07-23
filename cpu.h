#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "memory.h"

#define LITTLE_ENDIAN_SYS

#ifdef LITTLE_ENDIAN_SYS
#define DL 0
#define DH 1
#define AL 0
#define AH 1
#define BL 0
#define BH 1
#define CL 0
#define CH 1
#endif

#ifdef BIG_ENDIAN_SYS
#define DL 1
#define DH 0
#define AL 1
#define AH 0
#define BL 1
#define BH 0
#define CL 1
#define CH 0
#endif

// GENERAL PUPROSE REGISTERS
#define SR_AMOUNT 6
#define GPR_AMOUNT 9
#define BIT_MODE_16 16
#define BIT_MODE_32 32

/*
#define DL (0xFF & EDX)
#define DH ((EDX >> 8) & 0xFF)
#define AL (0xFF & EAX)
#define AH ((EAX >> 8) & 0xFF)
#define BL (0xFF & EBX)
#define BH ((EBX >> 8) & 0xFF)
#define CL (0xFF & ECX)
#define CH ((ECX >> 8) & 0xFF)
*/

typedef enum
{
    EAX,
    EBX,
    ECX,
    ESP, // top of stack address
    EBP, // defines current stack frame, points at the current local data. Also accesses routine params
    EDI,
    ESI,
    EDX,
    EIP,
    R_PLACEHOLDER
} gen_purpose_reg_type;
typedef union
{
    uint32_t dword;
    uint16_t word[2];
    uint8_t byte[4];
} GPRegister;

GPRegister gen_purpose_registers[GPR_AMOUNT]; //indexed by gpr_type 

/*
    ESP,  top of stack address
    EBP,  defines current stack frame, points at the current local data. Also accesses routine params

    DS register, holds the segment selector whose base address the CPU assumes for data loads/stores
        Used when a memory operand doesn;t specify another segment and doesn't use ESP/EBP as a base
    SS register, holds the selector for the stack segement
        Used at PUSH, POP, CALL, RET, ENTER/LEAVE, or any addressing mode whose base register
        is ESP or EBP

*/

typedef enum {
    CS, DS, ES, FS, GS, SS
} segment_reg_type;

typedef union {
    uint16_t word;
    uint8_t byte[2];
} SRegister;

SRegister segment_registers[SR_AMOUNT]; //indexed by gpr_type 

static inline uint16_t seg_reg_index(const SRegister *reg) {
    return reg->word >> 3;
}

static inline uint32_t gdt_limit(const gdt_entry *entry) {
    return (uint32_t)entry->limit_low | (((uint32_t)entry->flag_limit_high & 0x0F ) << 16);
}

static inline uint32_t gdt_base(const gdt_entry *entry) {
    return (uint32_t)entry->base_low | ((uint32_t)entry->base_mid << 16) | ((uint32_t)entry->base_high << 24);
}

int initialize_gen_purpose_registers();
int initialize_segment_registers();

#endif