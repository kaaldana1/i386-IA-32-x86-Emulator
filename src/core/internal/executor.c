#include "core/executor.h"

#define MOD_REGISTER 0x3
//=======================================================================================================================================================
//=========================================================PRINT HELPERS=================================================================================
void print_registers(CPU *cpu) {
    printf("+-----------------+----------+\n");
    printf("| Register        | Value    |\n");
    printf("+-----------------+----------+\n");
    printf("| EAX             | %08X |\n", cpu->gen_purpose_registers[0].dword);
    printf("| ECX             | %08X |\n", cpu->gen_purpose_registers[1].dword);
    printf("| EDX             | %08X |\n", cpu->gen_purpose_registers[2].dword);
    printf("| EBX             | %08X |\n", cpu->gen_purpose_registers[3].dword);
    printf("| EIP             | %08X |\n", cpu->gen_purpose_registers[4].dword);
    printf("| EBP             | %08X |\n", cpu->gen_purpose_registers[5].dword);
    printf("| ESI             | %08X |\n", cpu->gen_purpose_registers[6].dword);
    printf("| EDI             | %08X |\n", cpu->gen_purpose_registers[7].dword);
    printf("| ESP             | %08X |\n", cpu->gen_purpose_registers[8].dword);
    printf("+-----------------+----------+\n");
}

static void print_imm_reg(CPU *cpu,  Instruction *decoded_instr)
{
    printf("Immediate bytes: \n");
    for (int i = 0; i < decoded_instr->immediate_length; i++) {
        printf(" %02x  ", decoded_instr->immediate[i]);
    }
    printf("\n");
    print_registers(cpu);
    printf("===================EXECUTE DONE======================\n");
}


//=======================================================================================================================================================
//===================================================================General Utils=======================================================================

static inline uint32_t gpr32(CPU *cpu, GPR_type id)
{
    return cpu->gen_purpose_registers[id].dword;
}

static inline uint32_t gpr16(CPU *cpu, GPR_type id)
{
    return cpu->gen_purpose_registers[id].word[0];
}

static inline uint32_t gpr8(CPU *cpu, GPR_type id)
{
    return cpu->gen_purpose_registers[id].byte[0];
}

static inline uint32_t get_disp(Instruction *instr)
{
    switch (instr->displacement_length)
    {
        case 1: return (int32_t)instr->displacement[0];
        case 2: return (int32_t)((int16_t)(instr->displacement[0] | instr->displacement[1] << 8));
        case 3: return (int32_t)((uint32_t)instr->displacement[0] | (uint32_t)instr->displacement[1] << 8 | (uint32_t)instr->displacement[2] << 16 | (uint32_t)instr->displacement[3] << 24 );
        default: return 0;
    }
}

static inline uint32_t get_imm(Instruction *instr)
{
    return ((uint32_t)instr->immediate[0] | (uint32_t)instr->immediate[1] << 8 | (uint32_t)instr->immediate[2] << 16 | (uint32_t)instr->immediate[3] << 24);
}

static inline Operand_addr_form search_addr_form(Instruction *instr) 
{
    return  operand_addr_form_lut[instr->mod][instr->rm_field][instr->reg_or_opcode];
}

//=======================================================================================================================================================
//========================================================Effective Address Calculation Helpers==========================================================

static uint32_t calculate_EA(CPU *cpu, Instruction *instr)
{
    if (instr->mod == 0x3) 
        return 0; // register direct is used
    if (instr->mod == 0x00 && instr->rm_field == 0x05) 
        return get_disp(instr);

    uint32_t effective_addr = 0; 


    if (!instr->sib_length && !instr->displacement_length)
    {
        effective_addr = gpr32(cpu, search_addr_form(instr).effective_addr_register);
    }

    if (instr->sib_length)
    {
        if (!(instr->mod == 0 && instr->base == 0)) // base register exists
            effective_addr += gpr32(cpu, instr->base);
        if (instr->index != 0)
            effective_addr += (uint32_t)(gpr32(cpu, instr->index) << instr->scale);
    }

    if (instr->displacement_length != 0)
        effective_addr += get_disp(instr);

    return effective_addr;
    
}

//=======================================================================================================================================================
//===============================================================00 - 0F=================================================================================

int execute_ADD_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr) // 0x00
{
    (void)bus; (void)cpu; (void)decoded_instr;
    return 0;
}

int execute_ADD_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)  // 0x01
{
    printf("\n========Executing ADD_RM32_R32...============\n");
    uint32_t mem_value = 0;
    uint32_t effective_addr = calculate_EA(cpu, decoded_instr);
    Operand_addr_form addr_form = search_addr_form(decoded_instr);


    bus_read(bus, &mem_value, effective_addr);
    uint32_t result = mem_value + gpr32(cpu, addr_form.src_register);
    bus_write(bus, result, effective_addr);

#ifdef DEBUG
    print_registers(cpu);
    printf("Result is: %02x\n", result);
    printf("===================EXECUTE DONE======================\n");
#endif

    return 1;
}

int execute_ADD_R8_RM8 (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{ 
    printf("\n========Executing ADD_R8_RM8...============\n");
    uint32_t mem_value = 0;
    Operand_addr_form addr_form = operand_addr_form_lut[decoded_instr->mod][decoded_instr->rm_field][decoded_instr->reg_or_opcode];

    uint32_t value;
    if (decoded_instr->mod == MOD_REGISTER)
    {
        value = (uint32_t)cpu->gen_purpose_registers[addr_form.effective_addr_register].byte[0];
    }
    else 
    {
        uint32_t address = cpu->gen_purpose_registers[addr_form.effective_addr_register].dword;
        bus_read(bus, &value, address);
    }

    uint8_t result = cpu->gen_purpose_registers[addr_form.src_register].byte[0] + (uint8_t)value;
    cpu->gen_purpose_registers[addr_form.src_register].byte[0] = result;


#ifdef DEBUG
    printf("Result: %02x\n", result);
    print_registers(cpu);
    printf("===================EXECUTE DONE======================\n");
#endif
    return 0; 
}

int execute_ADD_R32_RM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_ADD_AL_IMM8 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_ADD_EAX_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }

// For the segment register ES
int execute_PUSH_ES(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_POP_ES(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }

//=======================================================================================================================================================
//===============================================================10 - 1F=================================================================================

// For the segment register SS
int execute_PUSH_SS(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_POP_SS(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }

//=======================================================================================================================================================
//===============================================================20 - 2F=================================================================================

int execute_AND_RM8_R8    (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_AND_RM32_R32  (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_AND_R8_RM8    (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_AND_R32_RM32  (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_AND_AL_IMM8   (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_AND_EAX_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_SEG_ES        (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_DAA           (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_CMP_RM8_R8    (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_CMP_RM32_R32  (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_CMP_R8_RM8    (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_CMP_R32_RM32  (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_CMP_AL_IMM8   (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_CMP_EAX_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }


//=======================================================================================================================================================
//===============================================================30 - 3F=================================================================================

int execute_XOR_RM8_R8     (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_XOR_RM32_R32   (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_XOR_R8_RM8     (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_XOR_R32_RM32   (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_XOR_AL_IMM8    (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_XOR_EAX_IMM32  (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }


//=======================================================================================================================================================
//===============================================================40 - 4F=================================================================================

int execute_INC_EAX_REX    (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  
int execute_INC_ECX_REX    (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  
int execute_INC_EDX_REX    (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  
int execute_INC_EBX_REX    (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  
int execute_INC_ESP_REX    (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  
int execute_INC_EBP_REX    (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  
int execute_INC_ESI_REX    (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  
int execute_INC_EDI_REX    (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  

//=======================================================================================================================================================
//===============================================================50 - 5F=================================================================================

int execute_PUSH_EAX(BUS *bus, CPU *cpu, Instruction *decoded_instr) {(void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_PUSH_ECX(BUS *bus, CPU *cpu, Instruction *decoded_instr) {(void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_PUSH_EDX(BUS *bus, CPU *cpu, Instruction *decoded_instr) {(void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_PUSH_EBX(BUS *bus, CPU *cpu, Instruction *decoded_instr) {(void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_PUSH_ESP(BUS *bus, CPU *cpu, Instruction *decoded_instr) {(void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_PUSH_EBP(BUS *bus, CPU *cpu, Instruction *decoded_instr) {(void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_PUSH_ESI(BUS *bus, CPU *cpu, Instruction *decoded_instr) {(void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_PUSH_EDI(BUS *bus, CPU *cpu, Instruction *decoded_instr) {(void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_POP_EAX(BUS *bus, CPU *cpu, Instruction *decoded_instr) {(void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_POP_ECX(BUS *bus, CPU *cpu, Instruction *decoded_instr) {(void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_POP_EDX(BUS *bus, CPU *cpu, Instruction *decoded_instr) {(void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_POP_EBX(BUS *bus, CPU *cpu, Instruction *decoded_instr) {(void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_POP_ESP(BUS *bus, CPU *cpu, Instruction *decoded_instr) {(void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_POP_EBP(BUS *bus, CPU *cpu, Instruction *decoded_instr) {(void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_POP_ESI(BUS *bus, CPU *cpu, Instruction *decoded_instr) {(void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_POP_EDI(BUS *bus, CPU *cpu, Instruction *decoded_instr) {(void) bus; (void)cpu; (void)decoded_instr; return 0; }

//=======================================================================================================================================================
//===============================================================60 - 6F=================================================================================

int execute_PUSHA(BUS *bus, CPU *cpu, Instruction *decoded_instr) { return 0; }
int execute_POPA(BUS *bus, CPU *cpu, Instruction *decoded_instr) { return 0; }
int execute_BOUND_GV_MA(BUS *bus, CPU *cpu, Instruction *decoded_instr) { return 0; }

//=======================================================================================================================================================
//===============================================================70 - 7F=================================================================================


//=======================================================================================================================================================
//===============================================================80 - 8F=================================================================================

int execute_IMM_GRP_EB_LB   (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  
int execute_IMM_GRP_EV_LZ   (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  
int execute_IMM_GRP_EV_LB   (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  

int execute_MOV_RM8_R8      (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  

int execute_MOV_RM32_R32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    printf("\n========Executing MOV_RM32_R32...============\n");
    Operand_addr_form addr_form = operand_addr_form_lut[decoded_instr->mod][decoded_instr->rm_field][decoded_instr->reg_or_opcode];
    bus_write(bus, cpu->gen_purpose_registers[addr_form.src_register].dword, cpu->gen_purpose_registers[addr_form.effective_addr_register].dword); 
#ifdef DEBUG
    printf("Register source: %02x\n", cpu->gen_purpose_registers[addr_form.src_register].dword);
    printf("Register: %d\n", addr_form.src_register);
    printf("EA Register: %d\n", addr_form.effective_addr_register);
    printf("===================EXECUTE DONE======================\n");
#endif
    return 1;
}

int execute_MOV_R8_RM8      (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  

int execute_MOV_R32_RM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    (void)bus; (void)decoded_instr;
#ifdef DEBUG
    printf("Executing MOV_R32_RM32\n");
    print_registers(cpu);
#endif
    return 1;
}

int execute_LEA(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }

//=======================================================================================================================================================
//===============================================================90 - 9F=================================================================================


//=======================================================================================================================================================
//===============================================================A0 - AF=================================================================================

int execute_MOV_AL_MOFFS8   (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  
int execute_MOV_EAXv_MOFFSv (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  
int execute_MOV_MOFFS8_AL   (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  
int execute_MOV_MOFFSv_EAXv (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  


//=======================================================================================================================================================
//===============================================================B0 - BF=================================================================================

int execute_MOV_AL_IMM8     (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  
int execute_MOV_CL_IMM8     (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  
int execute_MOV_DL_IMM8     (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  
int execute_MOV_R8_IMM8     (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }  

static int mov_reg_imm(CPU *cpu, GPR_type GPR, Instruction *decoded_instr)
{
    printf("\n========Executing MOV_EAX_IMM32...============\n");
   cpu->gen_purpose_registers[GPR].dword = *(uint32_t*)decoded_instr->immediate;
#ifdef DEBUG
    print_imm_reg(cpu, decoded_instr);
#endif
    return 1;
}

int execute_MOV_EAX_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    (void)bus;
    if (mov_reg_imm(cpu, EAX, decoded_instr)) { return 1; }
    return 0;
}

int execute_MOV_ECX_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) {
    (void)bus;
    if (mov_reg_imm(cpu, ECX, decoded_instr)) { return 1; }
    return 0;
}

int execute_MOV_EDX_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{ 
    (void)bus;
    if (mov_reg_imm(cpu, EDX, decoded_instr)) { return 1; }
    return 0;
}

int execute_MOV_EBX_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { 
    (void)bus;
    if (mov_reg_imm(cpu, EBX, decoded_instr)) { return 1; }
    return 0;
}

int execute_MOV_ESP_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{ 
    (void)bus;
    if (mov_reg_imm(cpu, ESP, decoded_instr)) { return 1; }
    return 0;
}

int execute_MOV_EBP_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr)
{ 
    (void)bus;
    if (mov_reg_imm(cpu, EBP, decoded_instr)) { return 1; }
    return 0;
}

int execute_MOV_ESI_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{ 
    (void)bus;
    if (mov_reg_imm(cpu, ESI, decoded_instr)) { return 1; }
    return 0;
}

int execute_MOV_EDI_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{ 
    (void)bus;
    if (mov_reg_imm(cpu, EDI, decoded_instr)) { return 1; }
    return 0;
}

//=======================================================================================================================================================
//===============================================================C0 - CF=================================================================================

int execute_RET (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }

//=======================================================================================================================================================
//===============================================================D0 - DF=================================================================================

int execute_SHIFT_EB_1   (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return 0; }    
int execute_SHIFT_EV_1   (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return 0; }    
int execute_SHIFT_EB_CL  (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return 0; }    
int execute_SHIFT_EV_CL  (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return 0; }    

//=======================================================================================================================================================
//===============================================================E0 - EF=================================================================================

int execute_CALL_REL32    (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return 0; }    
int execute_JMP_REL32     (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return 0; }    
int execute_INVALID0xEA   (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return 0; }    
int execute_JMP_REL8      (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return 0; }    

//=======================================================================================================================================================
//===============================================================F0 - FF=================================================================================

int execute_HLT (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    (void)bus;
    (void)decoded_instr;
    // raise HLT flag
    cpu->halt = 1;
    printf("\nProgram HALTED\n");
    return 1;
}

