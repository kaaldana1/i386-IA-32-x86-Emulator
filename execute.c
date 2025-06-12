#include "execute.h"

int execute_ADD_RM8_R8(Instruction *decoded_instr)
{
    return 0;
}

int execute_ADD_RM32_R32(Instruction *decoded_instr)
{
    printf("Executing ADDRM32R32...\n");
    return 1;
}

int execute_ADD_R8_RM8(Instruction *decoded_instr) { return 0; }
int execute_ADD_R32_RM32(Instruction *decoded_instr) { return 0; }
int execute_ADD_AL_IMM8(Instruction *decoded_instr) { return 0; }
int execute_ADD_EAX_IMM32(Instruction *decoded_instr) { return 0; }
