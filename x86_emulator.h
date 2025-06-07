#ifndef X86_EMULATOR_H
#define X86_EMULATOR_H

#define DEBUG
#define LITTLE_ENDIAN_SYS

#include "parse_code.h"
#include "opcode.h"

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
typedef enum {
 EAX, EBX, ECX, ESP,
 EBP, EDI, ESI, EDX,
 EIP
} Register_type;
typedef union {
    uint32_t dword;
    uint16_t word[2];
    uint8_t byte[4];
} Register;

Register registers[GPR_AMOUNT];

typedef struct {
    Register_type reg_1;
    Register_type reg_2;
} Operand_addressing_form;

// these are indexed by REG and RM values
const Operand_addressing_form operand_addr_form_lut[2][2][4] = { 
    {{{EAX, EAX}, {EAX, ECX}, {EAX, EDX}, {EAX, EBX} }, 
    {{ECX, EAX}, {ECX, ECX}, {ECX, EDX}, {ECX, EBX}} },

    {{{EAX, EAX}, {EAX, ECX}, {EAX, EDX}, {EAX, EBX} }, 
    {{ECX, EAX}, {ECX, ECX}, {ECX, EDX}, {ECX, EBX}} }
}; // will need to change this

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

int interpreter(Program *p);
Instruction * decoder(uint8_t *single_instr);
void put_byte(Instruction *decoded_instr, int field, uint8_t byte);


//=========
int execute_ADD_RM8_R8(Instruction *decoded_instr);
int execute_ADD_RM32_R32 (Instruction *decoded_instr);
int execute_ADD_R8_RM8 (Instruction *decoded_instr);
int execute_ADD_R32_RM32 (Instruction *decoded_instr);
int execute_ADD_AL_IMM8 (Instruction *decoded_instr);
int execute_ADD_EAX_IMM32(Instruction *decoded_instr);

// instead of switch statements, a table of function pointers, indexed by Opcode_ID
int (*execution_handler_lut[6]) (Instruction*) = { 
    execute_ADD_RM8_R8, execute_ADD_RM32_R32, execute_ADD_R8_RM8,
    execute_ADD_R32_RM32, execute_ADD_AL_IMM8, execute_ADD_EAX_IMM32
};

#endif