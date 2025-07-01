#include "execute.h"

int execute_ADD_RM8_R8(Instruction *decoded_instr)
{
    return 0;
}

int execute_ADD_RM32_R32(Instruction *decoded_instr) {

    Operand_addressing_form addr_form = operand_addr_form_lut[decoded_instr->mod][decoded_instr->reg_or_opcode][decoded_instr->rm_field];
    memory_write_dword(registers[addr_form.effective_addr_register].dword, registers[addr_form.src_register].dword); 
#ifdef DEBUG
    printf("Executing ADDRM32R32...\n");
    return 1;
#endif
}




int execute_ADD_AL_IMM8(Instruction *decoded_instr) { return 0; }

int execute_ADD_R8_RM8 (Instruction *decoded_instr) { return 0; }
int execute_ADD_R32_RM32 (Instruction *decoded_instr) { return 0; }
int execute_ADD_EAX_IMM32 (Instruction *decoded_instr) { return 0; }
int execute_INVALID (Instruction *decoded_instr) { return 0; }

int execute_MOV_R8_RM8 (Instruction *decoded_instr) { return 0; }

int execute_MOV_RM32_R32 (Instruction *decoded_instr) {
#ifdef DEBUG
    printf("Executing MOV_RM32_R32...\n");
#endif
    return 1;
}

int execute_MOV_RM8_R8 (Instruction *decoded_instr) { return 0; }

int execute_MOV_R32_RM32 (Instruction *decoded_instr) {
#ifdef DEBUG
    printf("Executing MOV_R32_RM32\n");
#endif
    Operand_addressing_form addr_form = operand_addr_form_lut[decoded_instr->mod][decoded_instr->reg_or_opcode][decoded_instr->rm_field];
    memory_write_dword(registers[addr_form.effective_addr_register].dword, registers[addr_form.src_register].dword); 
    return 1;
}

int execute_MOV_AL_MOFFS8 (Instruction *decoded_instr) { return 0; }
int execute_MOV_EAXv_MOFFSv (Instruction *decoded_instr) { return 0; }
int execute_MOV_MOFFS8_AL (Instruction *decoded_instr) { return 0; }
int execute_MOV_MOFFSv_EAXv(Instruction *decoded_instr) { return 0; }
int execute_MOV_AL_IMM8 (Instruction *decoded_instr) { return 0; }
int execute_MOV_CL_IMM8 (Instruction *decoded_instr) { return 0; }
int execute_MOV_DL_IMM8 (Instruction *decoded_instr) { return 0; }
int execute_MOV_R8_IMM8 (Instruction *decoded_instr) { return 0; }

int execute_MOV_EAX_IMM32(Instruction *decoded_instr) {
   registers[EAX].dword = *(uint32_t*)decoded_instr->immediate;
#ifdef DEBUG
    printf("Executing MOV_EAX_IMM32...\n");
    printf("Immediate bytes: \n");
    for (int i = 0; i < decoded_instr->immediate_length; i++) {
        printf(" %02x  ", decoded_instr->immediate[i]);
    }
   printf("Dword: %u\n", registers[EAX].dword);
#endif
    return 1;
}

int execute_MOV_ECX_IMM32 (Instruction *decoded_instr) {
   registers[ECX].dword = *(uint32_t*)decoded_instr->immediate;
#ifdef DEBUG
    printf("Executing MOV_ECX_IMM32...\n");
    printf("Immediate bytes: \n");
    for (int i = 0; i < decoded_instr->immediate_length; i++) {
        printf(" %02x  ", decoded_instr->immediate[i]);
    }
    return 1;
#endif
}

int execute_MOV_EDX_IMM32 (Instruction *decoded_instr) { return 0; }

int execute_MOV_EBX_IMM32 (Instruction *decoded_instr) { 
   registers[EBX].dword = *(uint32_t*)decoded_instr->immediate;
#ifdef DEBUG
    printf("Executing MOV_EBX_IMM32...\n");
    printf("Immediate bytes: \n");
    for (int i = 0; i < decoded_instr->immediate_length; i++) {
        printf(" %02x  ", decoded_instr->immediate[i]);
    }
    return 1;
#endif
}

int execute_MOV_ESP_IMM32 (Instruction *decoded_instr) { return 0; }
int execute_MOV_EBP_IMM32 (Instruction *decoded_instr) { return 0; }
int execute_MOV_ESI_IMM32 (Instruction *decoded_instr) { return 0; }
int execute_MOV_EDI_IMM32 (Instruction *decoded_instr) { return 0; }



int execute_ADC_RM8_R8(Instruction *decoded_instr) { return 0; }
int execute_ADC_RM32_R32(Instruction *decoded_instr) { return 0; }
int execute_ADC_R8_RM8(Instruction *decoded_instr) { return 0; }
int execute_ADC_R32_RM32(Instruction *decoded_instr) { return 0; }
int execute_ADC_AL_IMM8(Instruction *decoded_instr) { return 0; }
int execute_ADC_EAX_IMM32(Instruction *decoded_instr) { return 0; }
int execute_AND_RM8_R8(Instruction *decoded_instr) { return 0; }
int execute_AND_RM32_R32(Instruction *decoded_instr) { return 0; }
int execute_AND_R8_RM8(Instruction *decoded_instr) { return 0; }
int execute_AND_R32_RM32(Instruction *decoded_instr) { return 0; }
int execute_AND_AL_IMM8 (Instruction *decoded_instr) { return 0; }
int execute_AND_EAX_IMM32 (Instruction *decoded_instr) { return 0; }
int execute_XOR_RM8_R8 (Instruction *decoded_instr) { return 0; }
int execute_XOR_RM32_R32(Instruction *decoded_instr) { return 0; }
int execute_XOR_R8_RM8(Instruction *decoded_instr) { return 0; }
int execute_XOR_R32_RM32(Instruction *decoded_instr) { return 0; }
int execute_XOR_AL_IMM8(Instruction *decoded_instr) { return 0; }
int execute_XOR_EAX_IMM32(Instruction *decoded_instr) { return 0; }
int execute_NOP (Instruction *decoded_instr) { return 0; }
int execute_SUB_EAX_IMM32 (Instruction *decoded_instr) { return 0; }
int execute_PUSH_EAX (Instruction *decoded_instr) { return 0; }
int execute_PUSH_IMM8 (Instruction *decoded_instr) { return 0; }
int execute_POP_EAX (Instruction *decoded_instr) { return 0; }
int execute_POP_R32 (Instruction *decoded_instr) { return 0; }
int execute_JMP_REL32 (Instruction *decoded_instr) { return 0; }
int execute_CALL_REL32 (Instruction *decoded_instr) { return 0; }
int execute_RET (Instruction *decoded_instr) { return 0; }
