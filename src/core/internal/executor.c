#include "tables/decode_tables.h"
#include "core/alu.h"
#include "core/structs/flag_policy.h" 
#include "core/structs/instruction.h"
#include "ids/opcode_list.h"
#include "core/executor.h"

#define FOR_EACH_WIDTH(X) X(8) X(16) X(32)
#define FOR_EACH_REG(X) X(EAX) X(ECX) X(EDX) X(EBX) X(ESP) X(EBP) X(ESI) X(EDI)

#define MOD_REGISTER 0x3
//=======================================================================================================================================================
//=========================================================PRINT HELPERS=================================================================================
void print_registers(CPU *cpu) 
{
    printf("\n==================== CPU STATE ====================\n");

    // General-purpose registers
    printf("+-----------------+----------+\n"); 
    printf("| GPR             | Value    |\n"); 
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
    printf("\n");

    // Segment registers
    printf("+-----+------+------+------+------+------+------+\n");
    printf("| SR  |  CS  |  DS  |  ES  |  FS  |  GS  |  SS  |\n");
    printf("+-----+------+------+------+------+------+------+\n");
    printf("|Value| %04X | %04X | %04X | %04X | %04X | %04X |\n",
           cpu->segment_registers[CS].selector,
           cpu->segment_registers[DS].selector,
           cpu->segment_registers[ES].selector,
           cpu->segment_registers[FS].selector,
           cpu->segment_registers[GS].selector,
           cpu->segment_registers[SS].selector);
    printf("+-----+------+------+------+------+------+------+\n\n");

    // Status register / flags
    printf("+---------+----+----+----+----+----+----+\n");
    printf("|Stat reg | CF | PF | AF | ZF | SF | OF |\n");
    printf("+---------+----+----+----+----+----+----+\n");
    printf("|  bit    |  %d |  %d |  %d |  %d |  %d |  %d |\n",
           !!(cpu->status_register & (1 << 0)),  // CF bit 0
           !!(cpu->status_register & (1 << 2)),  // PF bit 2
           !!(cpu->status_register & (1 << 4)),  // AF bit 4
           !!(cpu->status_register & (1 << 6)),  // ZF bit 6
           !!(cpu->status_register & (1 << 7)),  // SF bit 7
           !!(cpu->status_register & (1 << 11))  // OF bit 11
    );
    printf("+---------+----+----+----+----+----+----+\n");

    printf("===================================================\n");
}

static void print_cell(uint8_t byte, uint32_t address) 
{
    printf("                +--------+ \n");
    printf(" Address %04X : | %02X     | \n", address, byte);
    printf("                +--------+ \n");
}

static void print_dword(uint32_t dword, uint32_t address) 
{
    for(size_t shift = 24; shift > 0; shift -= 8)
        print_cell((dword & (0x000000FF << shift)) >> shift, address++);

    print_cell((dword & 0x000000FF) >> 0, address++);
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
//===================================================================REGISTER ACCESSORS=======================================================================

//=======================================================================================================================================================
//===================================================================REGISTER ACCESSORS=======================================================================

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


//=======================================================================================================================================================
//===================================================================General Utils=======================================================================

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
            effective_addr += gpr32(cpu, instr->base); // register enums should match with base values
        if (instr->index != 0)
            effective_addr += (uint32_t)(gpr32(cpu, instr->index) << instr->scale);
    }

    if (instr->displacement_length != 0)
        effective_addr += get_disp(instr);

    return effective_addr;
    
}

//=======================================================================================================================================================
//===============================================================STATUS REGISTER UPDATE=================================================================================


Flag_policy get_flag_policy(Opclass opc) 
{
    if (opc & FP_ARITH_GRP_MASK) return FP_ARITH_GROUP;
    if (opc & FP_ARITH_2_GRP_MASK) return FP_ARITH_2_GROUP;
    if (opc & FP_INC_DEC_GRP_MASK) return FP_INC_DEC_GROUP;
    if (opc & FP_LOGIC_GRP_MASK) return FP_LOGIC_GRP;
    if (opc & FP_SHIFT_GRP_MASK) return FP_SHIFT_GRP;
    if (opc & FP_ROTATE_GRP_MASK) return FP_ROTATE_GRP;
}

static int update_status_register(CPU *cpu, Opclass opc, uint16_t possible_flags) 
{
    uint16_t permissible = possible_flags & get_flag_policy(opc).write;
    cpu->status_register &= ~permissible; 
    cpu->status_register |= (permissible & possible_flags); 
    return 1;
}


//=======================================================================================================================================================
//===============================================================00 - 0F=================================================================================

#define X(W)                                                                                        \
int execute_ADD_RM##W##_R##W(BUS *bus, CPU *cpu, Instruction *decoded_instr)                        \
{                                                                                                   \
    printf("===================EXECUTING ADD_RM%d_R%d======================\n", W, W);              \
    uint32_t mem_value = 0;                                                                         \
    uint32_t effective_addr = calculate_EA(cpu, decoded_instr);                                     \
    uint32_t source_value = (uint##W##_t)gpr##W(cpu, search_addr_form(decoded_instr).src_register); \
    bus_read(bus, &mem_value, effective_addr, W);                                                   \
    ALU_out out;                                                                                    \
    ALU(mem_value, source_value, 0, W, OPC_ADD, &out);                                              \
    update_status_register(cpu, OPC_ADD, out.flags_out);                                            \
    bus_write(bus, out.low, effective_addr, W);                                                     \
    print_registers(cpu);                                                                           \
    printf("Result is: %02x\n", out.low);                                                           \
    print_dword(out.low, effective_addr);                                                           \
    printf("===================EXECUTE DONE======================\n");                              \
    return 1; \
} 
FOR_EACH_WIDTH(X)                                           
#undef X

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
        bus_read(bus, &value, address, 8);
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
    bus_write(bus, cpu->gen_purpose_registers[addr_form.src_register].dword, cpu->gen_purpose_registers[addr_form.effective_addr_register].dword, 32); 
    print_dword(cpu->gen_purpose_registers[addr_form.src_register].dword, cpu->gen_purpose_registers[addr_form.effective_addr_register].dword);
    printf("===================EXECUTE DONE======================\n");
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


#define X(REG) \
int execute_MOV_##REG##_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr) \
{ \
    printf("===================EXECUTING MOV_%s_IMM32======================\n", #REG);  \
    (void)bus; \
    cpu->gen_purpose_registers[REG].dword = *(uint32_t*)decoded_instr->immediate; \
    print_imm_reg(cpu, decoded_instr); \
    return 1; \
} 

FOR_EACH_REG(X)
#undef X

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

