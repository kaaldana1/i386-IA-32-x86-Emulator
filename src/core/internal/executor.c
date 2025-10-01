#include "tables/decode_tables.h"
#include "core/alu.h"
#include "core/structs/FlagPolicy.h" 
#include "core/structs/instruction.h"
#include "ids/opcode_list.h"
#include "core/executor.h"

#define NO_CIN 0
#define CIN 1

#define OPC_THROW_RESULT_MASK ( OPC_CMP | OPC_TEST )
#define OPC_USES_CARRY_MASK (OPC_ADD | OPC_ADC | OPC_INC | OPC_MUL | OPC_IMUL )
#define OPC_USES_BORROW_MASK (OPC_SUB | OPC_SBB | OPC_CMP | OPC_DEC )
//=======================================================================================================================================================
//=========================================================OPCLASS RULES=================================================================================


//=======================================================================================================================================================
//=========================================================PRINT HELPERS=================================================================================
void print_registers(CPU *cpu) 
{
    printf("\n============================= CPU STATE ============================\n");

    printf("+-----------------+----------+ || +------+------ + || +----+----+\n");
    printf("|   GPR           |  Value   | || |  SR  | Value | || |Flag|Val |\n");
    printf("+-----------------+----------+ || +------+------ + || +----+----+\n");

    const char *gpr_names[] = {
        "EAX","ECX","EDX","EBX","EIP","EBP","ESI","EDI","ESP"
    };
    uint32_t gpr_vals[] = {
        cpu->gen_purpose_registers[EAX].dword,
        cpu->gen_purpose_registers[ECX].dword,
        cpu->gen_purpose_registers[EDX].dword,
        cpu->gen_purpose_registers[EBX].dword,
        cpu->gen_purpose_registers[EIP].dword,
        cpu->gen_purpose_registers[EBP].dword,
        cpu->gen_purpose_registers[ESI].dword,
        cpu->gen_purpose_registers[EDI].dword,
        cpu->gen_purpose_registers[ESP].dword
    };
    const char *sr_names[] = {"CS","DS","ES","FS","GS","SS"};
    uint16_t sr_vals[] = {
        cpu->segment_registers[CS].selector,
        cpu->segment_registers[DS].selector,
        cpu->segment_registers[ES].selector,
        cpu->segment_registers[FS].selector,
        cpu->segment_registers[GS].selector,
        cpu->segment_registers[SS].selector
    };
    const char *flag_names[] = {"CF","PF","AF","ZF","SF","OF"};
    int flag_vals[] = {
        !!(cpu->status_register & CF),
        !!(cpu->status_register & PF),
        !!(cpu->status_register & AF),
        !!(cpu->status_register & ZF),
        !!(cpu->status_register & SF),
        !!(cpu->status_register & OF)
    };

    int max_rows = 9; // gprs
    for (int i = 0; i < max_rows; i++) {
        // gpr column
        if (i < 9)
            printf("| %-15s | %08X |", gpr_names[i], gpr_vals[i]);
        else
            printf("| %-15s | %-8s |", "", "");

        // divider
        printf(" || ");

        // SR column
        if (i < 6)
            printf("| %-3s  | %04X  |", sr_names[i], sr_vals[i]);
        else
            printf("| %-3s  | %-4s  |", "", "");

        // divider
        printf(" || ");

        // flag column
        if (i < 6)
            printf("| %-3s | %d |", flag_names[i], flag_vals[i]);
        else
            printf("| %-3s | %-2s|", "", "");

        printf("\n");
    }

    printf("+-----------------+----------+ || +------+------ + || +----+----+\n");
    printf("\n====================================================================\n");
}


static void print_cell(uint8_t byte, uint32_t address) 
{
    printf("                +--------+ \n");
    printf(" Address %04X : | %02X     | \n", address, byte);
    printf("                +--------+ \n");
}

static void print_dword(uint32_t dword, uint32_t address) 
{
    for(size_t shift = 0; shift <= 24; shift += 8)
        print_cell((dword & (0x000000FF << shift)) >> shift, address++);
}

static void print_imm_reg(CPU *cpu,  Instruction *decoded_instr)
{
    printf("Immediate bytes (LSB -> GSB)\n");
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

static inline uint32_t gpr32(CPU *cpu, GeneralPurposeRegisterType id)
{
    return cpu->gen_purpose_registers[id].dword;
}

static inline uint32_t gpr16(CPU *cpu, GeneralPurposeRegisterType id)
{
    return cpu->gen_purpose_registers[id].word[0];
}

static inline uint32_t gpr8(CPU *cpu, GeneralPurposeRegisterType id)
{
    return cpu->gen_purpose_registers[id].byte[0];
}

static inline uint32_t gpr_w_handler(CPU *cpu, GeneralPurposeRegisterType id, size_t width)
{
    switch (width)
    {
        case 8: return gpr8(cpu, id);
        case 16: return gpr16(cpu, id);
        case 32: return gpr32(cpu, id);
        default: return 0;
    }
}

static inline void set_gpr32(CPU *cpu, GeneralPurposeRegisterType id, uint32_t value)
{
    cpu->gen_purpose_registers[id].dword = value;
}

static inline void set_gpr16(CPU *cpu, GeneralPurposeRegisterType id, uint16_t value)
{
    cpu->gen_purpose_registers[id].word[0] = value;
}

static inline void set_gpr8(CPU *cpu, GeneralPurposeRegisterType id, uint8_t value)
{
    cpu->gen_purpose_registers[id].byte[0] = value;
}

static inline void set_gpr_w_handler(CPU *cpu, GeneralPurposeRegisterType id, size_t width, uint32_t value)
{
    switch (width)
    {
        case 8: set_gpr8(cpu, id, (uint8_t)value); break;
        case 16: set_gpr16(cpu, id, (uint16_t)value); break;
        case 32: set_gpr32(cpu, id, value); break;
        default: break;
    }
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

//=======================================================================================================================================================
//========================================================Effective Address Calculation Helpers==========================================================

static uint32_t calculate_EA(CPU *cpu, Instruction *instr)
{

    if (instr->mod == 0x3) 
        return 0; // register direct is used
    if (instr->sib_length == 0 && instr->mod == 0x00 && instr->rm_field == 0x05) 
        return get_disp(instr); // disp32 only with no SIB

    uint32_t effective_addr =  0;
    uint32_t base = gpr32(cpu, instr->rm_field);

    if (instr->displacement_length != 0) 
        effective_addr += get_disp(instr);

    if (instr->sib_length) 
    {
        if (instr->index != 0x04) // has an index
            effective_addr += (uint32_t)(gpr32(cpu, instr->index) << instr->scale);
        
        if (!(instr->mod == 0x00 && instr->base == 0x05))
            base = gpr32(cpu, instr->base);
        else return effective_addr; // disp32 only WITH SIB
    }

    effective_addr += base; 
    return effective_addr;
    
}



//=======================================================================================================================================================
//===============================================================STATUS REGISTER UPDATE=================================================================================


FlagPolicy get_FlagPolicy(Opclass opc) 
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
    uint16_t borrow_bit = (possible_flags >> 9) & 0x1;
    possible_flags &= ((1 << 9) - 1); // truncate
    possible_flags = (opc & OPC_USES_BORROW_MASK) ? ((possible_flags & ~(1 << 2)) | (borrow_bit << 2) ) :
                                                    ((opc & OPC_USES_CARRY_MASK) ? possible_flags : 
                                                                                   possible_flags & ~(1 << 2));
    printf("Possible flags: %x\n", possible_flags);
    uint16_t permissible = get_FlagPolicy(opc).write;
    cpu->status_register &= ~permissible; 
    cpu->status_register |= (permissible & possible_flags); 
    return 1;
}

//=======================================================================================================================================================
//===============================================================ARITHMETIC AND LOGIC HELPERS=================================================================================

static int alu_two_op_rm_r(BUS *bus, CPU *cpu, Instruction *decoded_instr, size_t width, Opclass opclass, int cin)
{
    if(decoded_instr->mod == 0x3)
    {
        ALU_out out = { .low = 0, .high = 0, .flags_out = 0, .cin = 0 };
        uint32_t op1 = gpr_w_handler(cpu, decoded_instr->rm_field, width); 
        uint32_t op2 = gpr_w_handler(cpu, decoded_instr->reg_or_opcode, width);
        ALU(/*op1 register direct*/ op1, op2, cin, width, opclass, &out);


        if (!(opclass & OPC_THROW_RESULT_MASK))
            set_gpr_w_handler(cpu, decoded_instr->rm_field, width, out.low);
        update_status_register(cpu, opclass, out.flags_out);

        print_registers(cpu);                                                                                       
        printf("Result is: %02x\n", out.low);                                                                        
    }                                                                                                               
    else                                                                                                            
    {                                                                                                               
        // register indirect
        uint32_t mem_value = 0;
        uint32_t effective_addr = calculate_EA(cpu, decoded_instr);
        bus_read(bus, &mem_value, effective_addr, width);
        printf("Effective address calculated: %08X\n", effective_addr);
        printf("Value at effective address: %02x\n", mem_value);

        ALU_out out = { .cin = 0, .low = 0, .high = 0, .flags_out = 0 };

        uint32_t op2 = gpr_w_handler(cpu, decoded_instr->reg_or_opcode, width); 
        ALU(/*op1 from mem*/mem_value, /*op2 from reg*/ op2, cin, width, opclass, &out);                                                          

        if (!(opclass & OPC_THROW_RESULT_MASK)) 
        {
            bus_write(bus, out.low, effective_addr, width);
            print_dword(out.low, effective_addr);                                                                       
        }
        update_status_register(cpu, opclass, out.flags_out);                                                        

        print_registers(cpu);                                                                                       
        printf("Result is: %02x\n", out.low);                                                                       
    }                                                                                                               
    printf("===================EXECUTE DONE======================\n");                                              
    return 1; 
}

static int alu_two_op_r_rm(BUS *bus, CPU *cpu, Instruction *decoded_instr, size_t width, Opclass opclass, int cin)
{
    if (decoded_instr->mod == 0x3)                                                                                  
    {                                                                                                               
        ALU_out out = { .low = 0, .high = 0, .flags_out = 0, .cin = 0 };                                                        
        uint32_t op1 = gpr_w_handler(cpu, decoded_instr->reg_or_opcode, width); 
        uint32_t op2 = gpr_w_handler(cpu, decoded_instr->rm_field, width); 
        ALU(/*op1 register direct*/ op1, op2, cin, width, opclass, &out);
        if (!(opclass & OPC_THROW_RESULT_MASK))
            set_gpr_w_handler(cpu, decoded_instr->reg_or_opcode, out.low, width);
        update_status_register(cpu, opclass, out.flags_out);

        printf("Result: %02x\n", out.low);
    }                                                                                                               
    else                                                                                                            
    {                                                                                                               
        uint32_t effective_addr = calculate_EA(cpu, decoded_instr);                                                 
        uint32_t value = 0;                                                                                         
        bus_read(bus, &value, effective_addr, width);                                                                   

        ALU_out out = { .low = 0, .high = 0, .flags_out = 0, .cin = 0 };                                                      

        uint32_t op1 = gpr_w_handler(cpu, decoded_instr->reg_or_opcode, width); 
        ALU(/*op1 register dst*/ op1, /*op2 src from mem*/value, cin, width, opclass, &out);

        if (!(opclass & OPC_THROW_RESULT_MASK))
            set_gpr_w_handler(cpu, decoded_instr->reg_or_opcode, width, out.low);
        update_status_register(cpu, opclass, out.flags_out);

        printf("Result: %02x\n", out.low);                                                                           
    }                                                                                                               
    print_registers(cpu);                                                                                           
    printf("===================EXECUTE DONE======================\n");                                              
    return 0;                                                                                                       
}

//=======================================================================================================================================================
//===============================================================00 - 0F=================================================================================

int execute_ADD_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING ADD_RM8_R8======================\n");
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 8, OPC_ADD, NO_CIN);
}

int execute_ADD_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING ADD_RM32_R32======================\n");
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 32, OPC_ADD, NO_CIN);
}

int execute_ADD_R8_RM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING ADD_R8_RM8======================\n");
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 8, OPC_ADD, NO_CIN);
}

int execute_ADD_R32_RM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING ADD_R32_RM32======================\n");
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 32, OPC_ADD, NO_CIN);
}

int execute_ADD_AL_IMM8 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_ADD_EAX_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }

// For the segment register ES
int execute_PUSH_ES(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_POP_ES(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }


int execute_OR_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING OR_RM8_R8======================\n");
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 8, OPC_OR, NO_CIN);
}

int execute_OR_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING OR_RM32_R32======================\n");
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 32, OPC_OR, NO_CIN);
}

int execute_OR_R8_RM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING OR_R8_RM8======================\n");
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 8, OPC_OR, NO_CIN);
}

int execute_OR_R32_RM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING OR_R32_RM32======================\n");
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 32, OPC_OR, NO_CIN);
}


//=======================================================================================================================================================
//===============================================================10 - 1F=================================================================================

int execute_ADC_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING ADC_RM8_R8======================\n");
    
    uint32_t cin = (cpu->status_register >> 2) & 0x1;
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 8, OPC_ADC, cin);
}

int execute_ADC_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING ADC_RM32_R32======================\n");
    uint32_t cin = (cpu->status_register >> 2) & 0x1;
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 32, OPC_ADC, cin);
}

int execute_ADC_R8_RM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING ADC_R8_RM8======================\n");
    uint32_t cin = (cpu->status_register >> 2) & 0x1;
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 8, OPC_ADC, cin);
}

int execute_ADC_R32_RM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING ADC_R32_RM32======================\n");
    uint32_t cin = (cpu->status_register >> 2) & 0x1;
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 32, OPC_ADC, cin);
}


// For the segment register SS
int execute_PUSH_SS(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }
int execute_POP_SS(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return 0; }

int execute_SBB_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING SBB_RM8_R8======================\n");
    uint32_t cin = (cpu->status_register >> 2) & 0x1;
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 8, OPC_SBB, cin);
}

int execute_SBB_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING SBB_RM32_RM32======================\n");
    uint32_t cin = (cpu->status_register >> 2) & 0x1;
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 32, OPC_SBB, cin);
}

int execute_SBB_R8_RM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING SBB_R8_RM8======================\n");
    uint32_t cin = (cpu->status_register >> 2) & 0x1;
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 8, OPC_SBB, cin);
}

int execute_SBB_R32_RM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING SBB_R32_RM32======================\n");
    uint32_t cin = (cpu->status_register >> 2) & 0x1;
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 32, OPC_SBB, cin);
}


//=======================================================================================================================================================
//===============================================================20 - 2F=================================================================================
int execute_AND_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING AND_RM8_R8======================\n");
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 8, OPC_AND, NO_CIN);
}

int execute_AND_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING AND_RM32_R32======================\n");
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 32, OPC_AND, NO_CIN);
}

int execute_AND_R8_RM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING AND_R8_RM8======================\n");
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 8, OPC_AND, NO_CIN);
}

int execute_AND_R32_RM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING AND_R32_RM32======================\n");
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 32, OPC_AND, NO_CIN);
}


int execute_AND_AL_IMM8   (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_AND_EAX_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_SEG_ES        (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_DAA           (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }

int execute_SUB_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING SUB_RM8_R8======================\n");
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 8, OPC_SUB, NO_CIN);
}

int execute_SUB_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING SUB_RM32_R32======================\n");
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 32, OPC_SUB, NO_CIN);
}

int execute_SUB_R8_RM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING SUB_R8_RM8======================\n");
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 8, OPC_SUB, NO_CIN);
}

int execute_SUB_R32_RM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING SUB_R32_RM32======================\n");
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 32, OPC_SUB, NO_CIN);
}


//=======================================================================================================================================================
//===============================================================30 - 3F=================================================================================

int execute_XOR_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING XOR_RM8_R8======================\n");
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 8, OPC_XOR, NO_CIN);
}

int execute_XOR_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING XOR_RM32_R32======================\n");
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 32, OPC_XOR, NO_CIN);
}

int execute_XOR_R8_RM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING XOR_R8_RM8======================\n");
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 8, OPC_XOR, NO_CIN);
}

int execute_XOR_R32_RM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING XOR_R32_RM32======================\n");
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 32, OPC_XOR, NO_CIN);
}


int execute_XOR_AL_IMM8    (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_XOR_EAX_IMM32  (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }

int execute_CMP_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING CMP_RM8_R8======================\n");
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 8, OPC_CMP, NO_CIN);
}

int execute_CMP_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING CMP_RM32_R32======================\n");
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 32, OPC_CMP, NO_CIN);
}

int execute_CMP_R8_RM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING CMP_R8_RM8======================\n");
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 8, OPC_CMP, NO_CIN);
}

int execute_CMP_R32_RM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING CMP_R32_RM32======================\n");
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 32, OPC_CMP, NO_CIN);
}


int execute_CMP_AL_IMM8   (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }
int execute_CMP_EAX_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return 0; }


//=======================================================================================================================================================
//===============================================================40 - 4F=================================================================================

static int alu_increment_register(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    GeneralPurposeRegisterType destination_register = decoded_instr->opcode[0] & 0x7;
    
    ALU_out out;
    ALU(/*op1, base*/gpr_w_handler(cpu, destination_register, 32),
        /*op2, none*/0, 
                     NO_CIN, 32, OPC_INC, &out);
    
    set_gpr_w_handler(cpu, destination_register, 32, out.low);
    update_status_register(cpu, OPC_INC, out.flags_out);

    print_registers(cpu);
}

int execute_INC_EAX(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING INC_EAX======================\n");
    return alu_increment_register(bus, cpu, decoded_instr);
}

int execute_INC_ECX(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING INC_ECX======================\n");
    return alu_increment_register(bus, cpu, decoded_instr);
}

int execute_INC_EDX(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING INC_EDX======================\n");
    return alu_increment_register(bus, cpu, decoded_instr);
}

int execute_INC_EBX(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING INC_EBX======================\n");
    return alu_increment_register(bus, cpu, decoded_instr);
}

int execute_INC_ESP(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING INC_ESP======================\n");
    return alu_increment_register(bus, cpu, decoded_instr);
}

int execute_INC_EBP(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING INC_EBP======================\n");
    return alu_increment_register(bus, cpu, decoded_instr);
}

int execute_INC_ESI(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING INC_ESI======================\n");
    return alu_increment_register(bus, cpu, decoded_instr);
}

int execute_INC_EDI(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING INC_EDI======================\n");
    return alu_increment_register(bus, cpu, decoded_instr);
}


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

int execute_TEST_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING TEST_RM8_R8======================\n");
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 8, OPC_TEST, NO_CIN);
}

int execute_TEST_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING TEST_RM32_R32======================\n");
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 32, OPC_TEST, NO_CIN);
}

int execute_TEST_R8_RM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING TEST_R8_RM8======================\n");
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 8, OPC_TEST, NO_CIN);
}

int execute_TEST_R32_RM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING TEST_R32_RM32======================\n");
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 32, OPC_TEST, NO_CIN);
}

int execute_MOV_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    printf("\n========Executing MOV_RM8_R8...============\n");
    uint32_t effective_addr = calculate_EA(cpu, decoded_instr);
    uint32_t reg_src_value = gpr_w_handler(cpu, decoded_instr->reg_or_opcode, 8);

    if (decoded_instr->mod == 0x3) {
        // r/m is  a register, write to it
        set_gpr_w_handler(cpu, decoded_instr->rm_field, 8, reg_src_value);
        print_registers(cpu);
    } else {
        // r/m is memory, write to it
        bus_write(bus, reg_src_value, effective_addr, 8);
        print_cell((uint8_t)reg_src_value, effective_addr);
    }
    printf("===================EXECUTE DONE======================\n");
    return 1;
}

int execute_MOV_RM32_R32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    printf("\n========Executing MOV_RM32_R32...============\n");
    uint32_t reg_src_value = gpr_w_handler(cpu, decoded_instr->reg_or_opcode, 32);

    if (decoded_instr->mod == 0x3) {
        // r/m is  a register, write to it
        set_gpr_w_handler(cpu, decoded_instr->rm_field, 32, reg_src_value);
        print_registers(cpu);
    } else {
        // r/m is memory, write to it
        uint32_t effective_addr = calculate_EA(cpu, decoded_instr);
        bus_write(bus, reg_src_value, effective_addr, 32);
        print_dword(reg_src_value, effective_addr);
    }
    printf("===================EXECUTE DONE======================\n");
    return 1;
}

int execute_MOV_R8_RM8 (BUS *bus, CPU *cpu, Instruction *decoded_instr)    
{
    printf("\n========Executing MOV_R8_RM8...============\n");

    if (decoded_instr->mod == 0x3) 
    {
        set_gpr_w_handler(cpu, decoded_instr->reg_or_opcode, 8, gpr_w_handler(cpu, decoded_instr->rm_field, 8));
        printf("Value %x moved to register\n", gpr_w_handler(cpu, decoded_instr->rm_field, 8));
    }

    else 
    {
        uint32_t from_mem_value = 0;
        bus_read(bus, &from_mem_value, calculate_EA(cpu, decoded_instr), 8);
        set_gpr_w_handler(cpu, decoded_instr->reg_or_opcode, 8, from_mem_value);
        printf("Value %x moved to register\n", from_mem_value);
    }
    print_registers(cpu);
}

int execute_MOV_R32_RM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    printf("\n========Executing MOV_R32_RM32...============\n");

    if (decoded_instr->mod == 0x3) 
    {
        set_gpr_w_handler(cpu, decoded_instr->reg_or_opcode, 32, gpr_w_handler(cpu, decoded_instr->rm_field, 32));
        printf("Value %x moved to register\n", gpr_w_handler(cpu, decoded_instr->rm_field, 32));
    }

    else 
    {
        uint32_t from_mem_value = 0;
        bus_read(bus, &from_mem_value, calculate_EA(cpu, decoded_instr), 32);
        set_gpr_w_handler(cpu, decoded_instr->reg_or_opcode, 32, from_mem_value);
        printf("Value %x moved to register\n", from_mem_value);
    }
    print_registers(cpu);
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


int execute_MOV_EAX_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING MOV_EAX_IMM32======================\n");
    (void)bus;
    set_gpr_w_handler(cpu, EAX, 32, get_imm(decoded_instr));
    print_imm_reg(cpu, decoded_instr);
    return 1;
}

int execute_MOV_ECX_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING MOV_ECX_IMM32======================\n");
    (void)bus;
    set_gpr_w_handler(cpu, ECX, 32, get_imm(decoded_instr));
    print_imm_reg(cpu, decoded_instr);
    return 1;
}

int execute_MOV_EDX_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING MOV_EDX_IMM32======================\n");
    (void)bus;
    set_gpr_w_handler(cpu, EDX, 32, get_imm(decoded_instr));
    print_imm_reg(cpu, decoded_instr);
    return 1;
}

int execute_MOV_EBX_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING MOV_EBX_IMM32======================\n");
    (void)bus;
    set_gpr_w_handler(cpu, EBX, 32, get_imm(decoded_instr));
    print_imm_reg(cpu, decoded_instr);
    return 1;
}

int execute_MOV_ESP_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING MOV_ESP_IMM32======================\n");
    (void)bus;
    set_gpr_w_handler(cpu, ESP, 32, get_imm(decoded_instr));
    print_imm_reg(cpu, decoded_instr);
    return 1;
}

int execute_MOV_EBP_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING MOV_EBP_IMM32======================\n");
    (void)bus;
    set_gpr_w_handler(cpu, EBP, 32, get_imm(decoded_instr));
    print_imm_reg(cpu, decoded_instr);
    return 1;
}

int execute_MOV_ESI_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING MOV_ESI_IMM32======================\n");
    (void)bus;
    set_gpr_w_handler(cpu, ESI, 32, get_imm(decoded_instr));
    print_imm_reg(cpu, decoded_instr);
    return 1;
}

int execute_MOV_EDI_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    printf("===================EXECUTING MOV_EDI_IMM32======================\n");
    (void)bus;
    set_gpr_w_handler(cpu, EDI, 32, get_imm(decoded_instr));
    print_imm_reg(cpu, decoded_instr);
    return 1;
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

int execute_CLC (BUS *bus, CPU *cpu, Instruction *decoded_instr) //F9
{
    printf("===================EXECUTING CLC======================\n");
    cpu->status_register &= ~(CF);
    print_registers(cpu);
    return 1;
}

int execute_STC (BUS *bus, CPU *cpu, Instruction *decoded_instr) //F9
{
    printf("===================EXECUTING STC======================\n");
    cpu->status_register |= CF;
    print_registers(cpu);
    return 1;
}


int execute_HLT (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    (void)bus;
    (void)decoded_instr;
    // raise HLT flag
    printf("===================EXECUTING HALT======================\n");
    cpu->halt = 1;
    printf("\nProgram HALTED\n");
    return 1;
}

