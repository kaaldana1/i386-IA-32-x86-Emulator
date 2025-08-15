#include "core/executor.h"

void print_registers(CPU *cpu) {
    printf("==========================\n");
    printf("REGISTER CONTENTS: \n");

    printf("EAX: %02x\n", cpu->gen_purpose_registers[0].dword);
    printf("EBX: %02x\n", cpu->gen_purpose_registers[1].dword);
    printf("ECX: %02x\n", cpu->gen_purpose_registers[2].dword);
    printf("ESP: %02x\n", cpu->gen_purpose_registers[3].dword);
    printf("EBP: %02x\n", cpu->gen_purpose_registers[4].dword);
    printf("EDI: %02x\n", cpu->gen_purpose_registers[5].dword);
    printf("ESI: %02x\n", cpu->gen_purpose_registers[6].dword);
    printf("EDX: %02x\n", cpu->gen_purpose_registers[7].dword);
    printf("EIP: %02x\n", cpu->gen_purpose_registers[8].dword);
    printf("==========================\n");
}


int execute_ADD_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    (void)bus; (void)cpu; (void)decoded_instr;
    return 0;
}

int execute_ADD_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    printf("\n========Executing ADD_RM32_R32...============\n");
    uint32_t mem_value = 0;
    Operand_addr_form addr_form = operand_addr_form_lut[decoded_instr->mod][decoded_instr->rm_field][decoded_instr->reg_or_opcode];
    bus_read(bus, &mem_value, cpu->gen_purpose_registers[addr_form.effective_addr_register].dword);
    uint32_t result = mem_value + cpu->gen_purpose_registers[addr_form.src_register].dword;
    bus_write(bus, result, cpu->gen_purpose_registers[addr_form.effective_addr_register].dword);

#ifdef DEBUG
    print_registers(cpu);
    printf("Effective address: %02x\n", cpu->gen_purpose_registers[addr_form.effective_addr_register].dword);
    printf("Mem value: %02x\n", mem_value);
    printf("Register value: %02x\n", cpu->gen_purpose_registers[addr_form.src_register].dword);
    printf("Result: %02x\n", result);
    printf("===================DONE======================\n");
#endif

    return 1;
}




int execute_ADD_AL_IMM8(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr; return 0; }

int execute_ADD_R8_RM8 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_ADD_R32_RM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_ADD_EAX_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_INVALID (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }

int execute_MOV_R8_RM8 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }

int execute_MOV_RM32_R32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    printf("\n========Executing MOV_RM32_R32...============\n");
    Operand_addr_form addr_form = operand_addr_form_lut[decoded_instr->mod][decoded_instr->rm_field][decoded_instr->reg_or_opcode];
    bus_write(bus, cpu->gen_purpose_registers[addr_form.src_register].dword, cpu->gen_purpose_registers[addr_form.effective_addr_register].dword); 
#ifdef DEBUG
    printf("Register source: %02x\n", cpu->gen_purpose_registers[addr_form.src_register].dword);
    printf("Register: %d\n", addr_form.src_register);
    printf("EA Register: %d\n", addr_form.effective_addr_register);
    printf("===================DONE======================\n");
#endif
    return 1;
}

int execute_MOV_RM8_R8 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }

int execute_MOV_R32_RM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    (void)bus; (void)decoded_instr;
#ifdef DEBUG
    printf("Executing MOV_R32_RM32\n");
    print_registers(cpu);
#endif
    return 1;
}

int execute_MOV_AL_MOFFS8 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_MOV_EAXv_MOFFSv (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_MOV_MOFFS8_AL (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_MOV_MOFFSv_EAXv(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_MOV_AL_IMM8 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_MOV_CL_IMM8 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_MOV_DL_IMM8 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_MOV_R8_IMM8 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }

int execute_MOV_EAX_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    (void)bus;
    printf("\n========Executing MOV_EAX_IMM32...============\n");
   cpu->gen_purpose_registers[EAX].dword = *(uint32_t*)decoded_instr->immediate;
#ifdef DEBUG
    printf("Immediate bytes: \n");
    for (int i = 0; i < decoded_instr->immediate_length; i++) {
        printf(" %02x  ", decoded_instr->immediate[i]);
    }
   printf("Dword: %u\n", cpu->gen_purpose_registers[EAX].dword);
    print_registers(cpu);
    printf("===================DONE======================\n");
#endif
    return 1;
}

int execute_MOV_ECX_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) {
    (void)bus;
    printf("\n========Executing MOV_ECX_IMM32...============\n");
   cpu->gen_purpose_registers[ECX].dword = *(uint32_t*)decoded_instr->immediate;
#ifdef DEBUG
    printf("Immediate bytes: \n");
    for (int i = 0; i < decoded_instr->immediate_length; i++) {
        printf(" %02x  ", decoded_instr->immediate[i]);
    }
    printf("\n");
    print_registers(cpu);
    printf("===================DONE======================\n");
#endif
    return 1;
}

int execute_MOV_EDX_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr; return 0; }

int execute_MOV_EBX_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { 
    (void)bus;
    printf("\n========Executing MOV_EBX_IMM32...============\n");
   cpu->gen_purpose_registers[EBX].dword = *(uint32_t*)decoded_instr->immediate;
#ifdef DEBUG
    printf("Immediate bytes: \n");
    for (int i = 0; i < decoded_instr->immediate_length; i++) {
        printf(" %02x  ", decoded_instr->immediate[i]);
    }
    printf("\n");
    print_registers(cpu);
    printf("===================DONE======================\n");
#endif
    return 1;
}

int execute_MOV_ESP_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_MOV_EBP_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_MOV_ESI_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_MOV_EDI_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }



int execute_HLT (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    (void)bus;
    (void)decoded_instr;
    // raise HLT flag
    cpu->halt = 1;
    printf("\nProgram HALTED\n");
    return 1;
}
