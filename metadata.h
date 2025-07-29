#ifndef METADATA_H
#define METADATA_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define REG_TO_REG 0
#define REG_TO_IMM 1
#define REG_TO_MEM 2
#define MEM_TO_REG 3
#define IMM_TO_REG 4

#define HAS_MODRM true
#define NO_IMMEDIATE 0
#define SINGLE_BYTE_IMM 1
#define FOUR_BYTE_IMM 4
#define NO_MODRM false
#define PLACEHOLDER 0 

#include "opcode_list.h"

//---- LUTs for OPCODES----
//=========================
#undef X
typedef enum
{
#define X(name, _1, _2, _3, _4, _5) name,
    FOREACH_OPCODE(X)
#undef X
OPCODE_COUNT
} Opcode_ID;

typedef struct
{
    bool has_modrm;
    size_t immediate_bytes;
    size_t operand_count;
    int operand_type;
} Instruction_metadata;


static const Instruction_metadata instr_metadata_lut[OPCODE_COUNT] = { // also indexed by opcode id
#define X(_name, _op_byte, has_modrm, imm_bytes, operand_count, operand_type) {has_modrm, imm_bytes, operand_count, operand_type},
    FOREACH_OPCODE(X)
#undef X
};

#endif