#ifndef EXECUTE_H
#define EXECUTE_H

#include "decoder.h"

int execute_ADD_RM8_R8(Instruction *decoded_instr);
int execute_ADD_RM32_R32(Instruction *decoded_instr);
int execute_ADD_R8_RM8(Instruction *decoded_instr);
int execute_ADD_R32_RM32(Instruction *decoded_instr);
int execute_ADD_AL_IMM8(Instruction *decoded_instr);
int execute_ADD_EAX_IMM32(Instruction *decoded_instr);

// instead of switch statements, a table of function pointers, indexed by Opcode_ID
static int (*execution_handler_lut[6])(Instruction *) = {
    execute_ADD_RM8_R8, execute_ADD_RM32_R32, execute_ADD_R8_RM8,
    execute_ADD_R32_RM32, execute_ADD_AL_IMM8, execute_ADD_EAX_IMM32};

#endif