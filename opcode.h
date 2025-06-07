#ifndef OPCODE_H
#define OPCODE_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define REG_TO_REG 0
#define REG_TO_IMM 1
#define REG_TO_MEM 2
#define MEM_TO_REG 3

#define HAS_MODRM true
#define HAS_IMMEDIATE true
#define HAS_SIB true
#define HAS_DISPLACEMENT true
#define NO_MODRM false
#define NO_IMMEDIATE false
#define NO_SIB false
#define NO_DISPLACEMENT false

#define md_ADD_RM8_R8  {HAS_MODRM,  NO_IMMEDIATE, NO_SIB, NO_DISPLACEMENT, 2, REG_TO_MEM}
#define md_ADD_RM32_R32  {HAS_MODRM, NO_IMMEDIATE, NO_SIB, NO_DISPLACEMENT, 2, MEM_TO_REG}
#define md_ADD_R32_RM32  {HAS_MODRM, NO_IMMEDIATE, NO_SIB, NO_DISPLACEMENT, 2, REG_TO_MEM}
#define md_ADD_AL_IMM8  {NO_MODRM, HAS_IMMEDIATE, NO_SIB, 2, NO_DISPLACEMENT, REG_TO_MEM}
#define md_ADD_EAX_IMM32  {NO_MODRM, HAS_IMMEDIATE, NO_SIB, 2, NO_DISPLACEMENT, REG_TO_MEM} //change
#define md_ADD_R8_RM8  {NO_MODRM, HAS_IMMEDIATE, NO_SIB, 2, NO_DISPLACEMENT, REG_TO_MEM} //change

//---- LUTs for OPCODES----
//=========================
typedef enum {

    ADD_RM8_R8, ADD_RM32_R32, ADD_R8_RM8,
    ADD_R32_RM32, ADD_AL_IMM8, ADD_EAX_IMM32,

    INVALID,

    ADC_RM8_R8, ADC_RM32_R32, ADC_R8_RM8,
    ADC_R32_RM32, ADC_AL_IMM8, ADC_EAX_IMM32,


    AND_RM8_R8, AND_RM32_R32, AND_R8_RM8,
    AND_R32_RM32, AND_AL_IMM8, AND_EAX_IMM32,

    XOR_RM8_R8, XOR_RM32_R32, XOR_R8_RM8,
    XOR_R32_RM32, XOR_AL_IMM8, XOR_EAX_IMM32,
    
    NOP, 

    MOV_R32_IMM32, MOV_RM32_R32, MOVE_R32_RM32,

    SUB_EAX_IMM32,
    PUSH_EAX, PUSH_IMM8, 
    POP_EAX, POP_R32,
    JMP_REL32,
    CALL_REL32,
    RET
} Opcode_ID;


Opcode_ID single_byte_opcode_lut[6] = {  // this should be expanded to 256 entries once completed (some will be invalid)
    ADD_RM8_R8, ADD_RM32_R32, ADD_R8_RM8, // 0x00, 0x01, 0x02
    ADD_R32_RM32, ADD_AL_IMM8, ADD_EAX_IMM32, // 0x03, 0x04, 0x05
};


typedef struct {
    bool has_modrm;
    bool has_immediate;
    bool has_sib;
    bool has_displacement;
    size_t operand_count;
    int operand_type;
} Instruction_metadata;


static const Instruction_metadata instr_metadata_lut[6] = { // also indexed by opcode id
    md_ADD_RM8_R8, md_ADD_RM32_R32, md_ADD_R8_RM8, 
    md_ADD_R32_RM32, md_ADD_AL_IMM8, md_ADD_EAX_IMM32
};

#endif