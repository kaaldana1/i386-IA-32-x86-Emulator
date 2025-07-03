#ifndef EXECUTE_H
#define EXECUTE_H
#include "decoder.h"
#include "memory.h"

void print_registers();
int execute_ADD_RM8_R8 (Instruction *decoded_instr);
int execute_ADD_RM32_R32 (Instruction *decoded_instr);
int execute_ADD_R8_RM8 (Instruction *decoded_instr);
int execute_ADD_R32_RM32 (Instruction *decoded_instr);
int execute_ADD_AL_IMM8 (Instruction *decoded_instr);
int execute_ADD_EAX_IMM32 (Instruction *decoded_instr);
int execute_INVALID (Instruction *decoded_instr);
int execute_MOV_R8_RM8 (Instruction *decoded_instr);
int execute_MOV_RM32_R32 (Instruction *decoded_instr);
int execute_MOV_RM8_R8 (Instruction *decoded_instr);
int execute_MOV_R32_RM32 (Instruction *decoded_instr);
int execute_MOV_AL_MOFFS8 (Instruction *decoded_instr);
int execute_MOV_EAXv_MOFFSv (Instruction *decoded_instr);
int execute_MOV_MOFFS8_AL (Instruction *decoded_instr);
int execute_MOV_MOFFSv_EAXv(Instruction *decoded_instr);
int execute_MOV_AL_IMM8 (Instruction *decoded_instr);
int execute_MOV_CL_IMM8 (Instruction *decoded_instr);
int execute_MOV_DL_IMM8 (Instruction *decoded_instr);
int execute_MOV_R8_IMM8 (Instruction *decoded_instr);
int execute_MOV_EAX_IMM32(Instruction *decoded_instr);
int execute_MOV_ECX_IMM32 (Instruction *decoded_instr);
int execute_MOV_EDX_IMM32 (Instruction *decoded_instr);
int execute_MOV_EBX_IMM32 (Instruction *decoded_instr);
int execute_MOV_ESP_IMM32 (Instruction *decoded_instr);
int execute_MOV_EBP_IMM32 (Instruction *decoded_instr);
int execute_MOV_ESI_IMM32 (Instruction *decoded_instr);
int execute_MOV_EDI_IMM32 (Instruction *decoded_instr);
int execute_ADC_RM8_R8(Instruction *decoded_instr);
int execute_ADC_RM32_R32(Instruction *decoded_instr);
int execute_ADC_R8_RM8(Instruction *decoded_instr);
int execute_ADC_R32_RM32(Instruction *decoded_instr);
int execute_ADC_AL_IMM8(Instruction *decoded_instr);
int execute_ADC_EAX_IMM32(Instruction *decoded_instr);
int execute_AND_RM8_R8(Instruction *decoded_instr);
int execute_AND_RM32_R32(Instruction *decoded_instr);
int execute_AND_R8_RM8(Instruction *decoded_instr);
int execute_AND_R32_RM32(Instruction *decoded_instr);
int execute_AND_AL_IMM8 (Instruction *decoded_instr);
int execute_AND_EAX_IMM32 (Instruction *decoded_instr);
int execute_XOR_RM8_R8 (Instruction *decoded_instr);
int execute_XOR_RM32_R32(Instruction *decoded_instr);
int execute_XOR_R8_RM8(Instruction *decoded_instr);
int execute_XOR_R32_RM32(Instruction *decoded_instr);
int execute_XOR_AL_IMM8(Instruction *decoded_instr);
int execute_XOR_EAX_IMM32(Instruction *decoded_instr);
int execute_NOP (Instruction *decoded_instr);
int execute_SUB_EAX_IMM32 (Instruction *decoded_instr);
int execute_PUSH_EAX (Instruction *decoded_instr);
int execute_PUSH_IMM8 (Instruction *decoded_instr);
int execute_POP_EAX (Instruction *decoded_instr);
int execute_POP_R32 (Instruction *decoded_instr);
int execute_JMP_REL32 (Instruction *decoded_instr);
int execute_CALL_REL32 (Instruction *decoded_instr);
int execute_RET (Instruction *decoded_instr);

// instead of switch statements, a table of function pointers, indexed by Opcode_ID
static int (*execution_handler_lut[256])(Instruction *) = {
    execute_ADD_RM8_R8,
    execute_ADD_RM32_R32,
    execute_ADD_R8_RM8,
    execute_ADD_R32_RM32,
    execute_ADD_AL_IMM8,
    execute_ADD_EAX_IMM32,

    execute_INVALID,

    execute_MOV_R8_RM8, 
    execute_MOV_RM32_R32, 
    execute_MOV_RM8_R8, 
    execute_MOV_R32_RM32,

    execute_MOV_AL_MOFFS8, 
    execute_MOV_EAXv_MOFFSv, 
    execute_MOV_MOFFS8_AL, 
    execute_MOV_MOFFSv_EAXv,
    execute_MOV_AL_IMM8, 
    execute_MOV_CL_IMM8, 
    execute_MOV_DL_IMM8, 
    execute_MOV_R8_IMM8,
    execute_MOV_EAX_IMM32, 
    execute_MOV_ECX_IMM32, 
    execute_MOV_EDX_IMM32, 
    execute_MOV_EBX_IMM32, 
    execute_MOV_ESP_IMM32, 
    execute_MOV_EBP_IMM32, 
    execute_MOV_ESI_IMM32, 
    execute_MOV_EDI_IMM32, 

    execute_ADC_RM8_R8,
    execute_ADC_RM32_R32,
    execute_ADC_R8_RM8,
    execute_ADC_R32_RM32,
    execute_ADC_AL_IMM8,
    execute_ADC_EAX_IMM32,
    execute_AND_RM8_R8,
    execute_AND_RM32_R32,
    execute_AND_R8_RM8,
    execute_AND_R32_RM32,
    execute_AND_AL_IMM8,
    execute_AND_EAX_IMM32,
    execute_XOR_RM8_R8,
    execute_XOR_RM32_R32,
    execute_XOR_R8_RM8,
    execute_XOR_R32_RM32,
    execute_XOR_AL_IMM8,
    execute_XOR_EAX_IMM32,

    execute_NOP,

    execute_SUB_EAX_IMM32,
    execute_PUSH_EAX,
    execute_PUSH_IMM8,
    execute_POP_EAX,
    execute_POP_R32,
    execute_JMP_REL32,
    execute_CALL_REL32,
    execute_RET

};

#endif