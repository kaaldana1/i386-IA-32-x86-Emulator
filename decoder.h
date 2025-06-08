#ifndef DECODER_H
#define DECODER_H

#include "metadata.h"
#include "cpu.h"

#define OPERAND_SIZE_OVERRIDE 0x66
#define ADDR_SIZE_OVERRIDE 0x67
#define REP_REPE 0xF3
#define LOCK 0xF0
#define CS 0x2E
#define SS 0x36
#define DS 0x3E
#define ES 0x26
#define FS 0x64
#define GS 0x65

#define MAX_INSTR_LENGTH 15
#define MAX_PREFIX_LENGTH 4
#define MAX_OPCODE_LENGTH 3
#define MAX_MODRM_LENGTH 1
#define MAX_SIB_LENGTH 1
#define MAX_DISPLACEMENT_LENGTH 1
#define MAX_IMMEDIATE_LENGTH 4


#define MULT_BYTE_FLAG 0x0F 

#define FIELD_PREFIX 0
#define FIELD_OPCODE 1
#define FIELD_MODRM 2
#define FIELD_SIB 3
#define FIELD_DISPLACEMENT 4
#define FIELD_IMMEDIATE 5


typedef struct {
    Register_type reg_1;
    Register_type reg_2;
} Operand_addressing_form;

// these are indexed by REG and RM values
static const Operand_addressing_form operand_addr_form_lut[2][2][4] = { 
    {{{EAX, EAX}, {EAX, ECX}, {EAX, EDX}, {EAX, EBX} }, 
    {{ECX, EAX}, {ECX, ECX}, {ECX, EDX}, {ECX, EBX}} },

    {{{EAX, EAX}, {EAX, ECX}, {EAX, EDX}, {EAX, EBX} }, 
    {{ECX, EAX}, {ECX, ECX}, {ECX, EDX}, {ECX, EBX}} }
}; // will need to change this

static const Opcode_ID single_byte_opcode_lut[6] = {  // this should be expanded to 256 entries once completed (some will be invalid)
    ADD_RM8_R8, ADD_RM32_R32, ADD_R8_RM8, // 0x00, 0x01, 0x02
    ADD_R32_RM32, ADD_AL_IMM8, ADD_EAX_IMM32, // 0x03, 0x04, 0x05
};

/*
Opcode_ID two_byte_opcode_lut[]{}
*/


typedef struct { 
    uint8_t prefix[4]; uint8_t prefix_length;
      bool has_operand_size_override; bool has_addr_size_override;
      bool has_rep; bool has_lock; bool has_segment_override;
        bool has_CS; bool has_SS; bool has_DS;
        bool has_ES; bool has_FS; bool has_GS;

    uint8_t opcode[3]; uint8_t opcode_length;
      Opcode_ID opcode_id;

    uint8_t modrm; uint8_t modrm_length;
      uint8_t mod, reg_or_opcode, rm_field;
        Operand_addressing_form operands;

    uint8_t sib;  uint8_t sib_length;
    
    uint8_t displacement[4]; uint8_t displacement_length;

    uint8_t immediate[4]; uint8_t immediate_length;

    uint8_t total_length;
} Instruction;

static void put_byte(Instruction *decoded_instr, int field, uint8_t byte) {
    switch(field) {
        case FIELD_PREFIX: decoded_instr->prefix[decoded_instr->prefix_length++] = byte;
            break;
        case FIELD_OPCODE: decoded_instr->opcode[decoded_instr->opcode_length++] = byte;
            break;
        case FIELD_MODRM: decoded_instr->modrm = byte; decoded_instr->modrm_length++;
            break;
            case FIELD_SIB: decoded_instr->sib = byte; decoded_instr->sib_length++;
            break;
        case FIELD_DISPLACEMENT: decoded_instr->displacement[decoded_instr->displacement_length++]= byte;
            break;
        case FIELD_IMMEDIATE: decoded_instr->immediate[decoded_instr->immediate_length++] = byte;
            break;
        default:
            // bruh
            break;
    }
}

Instruction * decoder(uint8_t *single_instr);

#endif