#ifndef X86_EMULATOR_H
#define X86_EMULATOR_H

#define DEBUG
#define LITTLE_ENDIAN_SYS

#include "parse_code.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
// GENERAL PUPROSE REGISTERS
// All registers are instantiated with 32 bits
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

// Legacy Prefixes 
// For size directives
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

#define FIELD_PREFIX 0
#define FIELD_OPCODE 1
#define FIELD_MODRM 2
#define FIELD_SIB 3
#define FIELD_DISPLACEMENT 4
#define FIELD_IMMEDIATE 5

#define MULT_BYTE_FLAG 0x0F 


// EIP is the instruction pointer
// The rest are GPRs
typedef enum {
 EAX , EBX , ECX , ESP ,
 EBP , EDI , ESI , EDX ,
 EIP
} Register_type;

typedef union {
    uint32_t dword;
    uint16_t word[2];
    uint8_t byte[4];
} Register;

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


//-----------------------

typedef struct { 
    uint8_t prefix[4]; uint8_t prefix_length;
      bool has_operand_size_override; bool has_addr_size_override;
      bool has_rep; bool has_lock; bool has_segment_override;
        bool has_CS; bool has_SS; bool has_DS;
        bool has_ES; bool has_FS; bool has_GS;

    uint8_t opcode[3]; uint8_t opcode_length;
      Opcode_ID opcode_id;

    uint8_t modrm; uint8_t modrm_length;
    
    uint8_t sib; uint8_t sib_length;
    
    uint8_t displacement[4]; uint8_t displacement_length;

    uint8_t immediate[4]; uint8_t immediate_length;

    uint8_t total_length;
} Instruction;

Register registers[GPR_AMOUNT];

int interpreter(Program *p);
Instruction * decoder(uint8_t *single_instr);
void put_byte(Instruction *decoded_instr, int field, uint8_t byte);

int interpreter(Program *p) {
    int start_addr;
    while (registers[EIP].dword != p->program_length) {
        start_addr = registers[EIP].dword;

        uint8_t instr_buff[MAX_INSTR_LENGTH];
        memset(instr_buff, 0, sizeof(uint8_t));

        // Assume MAX instruction length, and truncate accordingly
        memcpy(instr_buff, (p->program + start_addr), 15);
        Instruction *decoded_instruction = decoder(instr_buff);
        break;
    }
    return 0;
}

Instruction * decoder(uint8_t *single_instr) {
    Instruction *decoded_instr = malloc(sizeof(Instruction));
    if (decoded_instr == NULL) { printf("nah"); exit(1); }
    memset(decoded_instr, 0, sizeof(Instruction));

    // Decode byte by byte 
    uint8_t byte_p = 0;
    while(byte_p < MAX_PREFIX_LENGTH) {
        bool add_byte = true;
        switch(single_instr[byte_p]) {
            case OPERAND_SIZE_OVERRIDE: decoded_instr->has_operand_size_override = true; add_byte = true;
                break;
            case ADDR_SIZE_OVERRIDE: decoded_instr->has_addr_size_override = true; add_byte = true;
                break;
            case REP_REPE: decoded_instr->has_rep = true; add_byte = true;
                break;
            case LOCK: decoded_instr->has_lock = true;
                break;
            case CS: // deal with this later on
                break;
            default:
                add_byte = false;
                break;
        }

        if (add_byte) {
            put_byte(decoded_instr, FIELD_PREFIX, single_instr[byte_p]);
            byte_p++;
        } else { break; }
    }

    while (byte_p < MAX_OPCODE_LENGTH) {
        // use lookup table
        if (!(single_instr[byte_p] == MULT_BYTE_FLAG)) {
            Opcode_ID opcode = single_byte_opcode_lut[single_instr[byte_p]];

            #ifdef DEBUG
                printf("\nOPCODE is: %d\n", opcode);
                printf("Byte_p is: %hhu\n", byte_p);
                printf("Undecoded byte: %02x\n", single_instr[byte_p]);
            #endif

            put_byte(decoded_instr, FIELD_OPCODE, single_instr[byte_p]);
            decoded_instr->opcode_id = opcode;

        } else { 
            // multiple-byte instructions here
        }
        break;
    }
/*
    while (byte_p < MAX_MODRM_LENGTH) {

    }
    
    while (byte_p < MAX_SIB_LENGTH) {

    }

    while (byte_p < MAX_DISPLACEMENT_LENGTH) {

    }

    while(byte_p < MAX_IMMEDIATE_LENGTH) {

    }
    */
   return decoded_instr;
}

void put_byte(Instruction *decoded_instr, int field, uint8_t byte) {
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






#endif