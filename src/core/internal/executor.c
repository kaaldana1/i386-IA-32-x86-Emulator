#include "tables/decode_tables.h"
#include "core/alu.h"
#include "core/structs/FlagPolicy.h" 
#include "core/structs/instruction.h"
#include "ids/opcode_list.h"
#include "core/executor.h"
#include "core/int_utils.h"
#include "ui/display_api.h"
#include "ids/return_code_list.h"

#define NO_CIN 0
#define CIN 1

#define OPC_THROW_RESULT_MASK ( OPC_CMP | OPC_TEST )
#define OPC_USES_CARRY_MASK (OPC_ADD | OPC_ADC | OPC_INC | OPC_MUL | OPC_IMUL )
#define OPC_USES_BORROW_MASK (OPC_SUB | OPC_SBB | OPC_CMP | OPC_DEC )

//=======================================================================================================================================================
//=========================================================OPCLASS RULES=================================================================================


//=======================================================================================================================================================
//=========================================================PRINT HELPERS=================================================================================


static void print_cell(uint8_t byte, uint32_t address) 
{
    uint8_t *byte_buffer = (uint8_t *)malloc(4);
    byte_buffer[0] = byte;
    
    if (ui_on)
        machine_state.ui_callbacks.ui_copy_mem_after_execute(byte_buffer, address);
}

static void print_dword(uint32_t dword, uint32_t address) 
{
    uint8_t *byte_buffer = (uint8_t *)malloc(4);
    for(size_t shift = 0; shift <= 24; shift += 8)
        byte_buffer[shift/8] = (uint8_t)(dword & ((uint32_t)0xFF << shift) >> shift);
    
    if (ui_on)
        machine_state.ui_callbacks.ui_copy_mem_after_execute(byte_buffer, address);
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
    return (FlagPolicy){0,0,0,0,0};
}

static int update_status_register(CPU *cpu, Opclass opc, uint16_t possible_flags) 
{
    uint16_t borrow_bit = (possible_flags >> 9) & 0x1;
    possible_flags &= ((1 << 9) - 1); // truncate
    possible_flags = (opc & OPC_USES_BORROW_MASK) ? ((possible_flags & ~(1 << 2)) | (borrow_bit << 2) ) :
                                                    ((opc & OPC_USES_CARRY_MASK) ? possible_flags : 
                                                                                   possible_flags & ~(1 << 2));
    uint16_t permissible = get_FlagPolicy(opc).write;
    cpu->status_register &= ~permissible; 
    cpu->status_register |= (permissible & possible_flags); 
    return 1;
}


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

static inline uint32_t gpr8(CPU *cpu, GeneralPurposeRegisterType id_base, GPR8 id)
{
    if (id <= 4)
        return cpu->gen_purpose_registers[id_base].byte[0];
    else 
        return cpu->gen_purpose_registers[id_base].byte[1];
}

static inline int get_base_reg(GeneralPurposeRegisterType *id_base, GPR8 id)
{
    if (id == AL || id == AH) { *id_base = EAX; return EXECUTE_SUCCESS; }
    if (id == CL || id == CH) { *id_base = ECX; return EXECUTE_SUCCESS; }
    if (id == DL || id == DH) { *id_base = EDX; return EXECUTE_SUCCESS; }
    if (id == BL || id == BH) { *id_base = EBX; return EXECUTE_SUCCESS; }
    else return INVALID_OPCODE;
}


static inline uint32_t gpr_w_handler(CPU *cpu, GeneralPurposeRegisterType id, size_t width)
{
    switch (width)
    {
        case 8: 
        {
            GeneralPurposeRegisterType id_base;
            GPR8 id8 = (GPR8)(id);
            get_base_reg(&id_base, id8);
            return gpr8(cpu, id_base, id8);
            break;
        }
        case 16: return gpr16(cpu, id); break;
        case 32: return gpr32(cpu, id); break;
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
        case 8: 
        {
            GPR8 id8 = (GPR8)(id);
            GeneralPurposeRegisterType id_base;
            get_base_reg(&id_base, id8);
            set_gpr8(cpu, id, (uint8_t)value); 
            break;
        }
        case 16: set_gpr16(cpu, id, (uint16_t)value); break;
        case 32: set_gpr32(cpu, id, value); break;
        default: break;
    }
}

//=======================================================================================================================================================
//===================================================================General Utils=======================================================================

// decoder stores immediates and displacements into array. casting the byte array into a dword is cpu-dependent... we want to ensure little-endianness
static inline uint32_t little_endian_to_uint32(uint8_t *bytes, size_t length)
{
    switch (length)
    {
        case 1: return (uint32_t)bytes[0];
        case 2: return (uint32_t)((uint16_t)(bytes[0] | bytes[1] << 8));
        case 4: return (uint32_t)((uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24 );
        default: return 0;
    }
}

static inline int32_t little_endian_to_int32(uint8_t *bytes, size_t length)
{
    switch (length)
    {
        case 1: return (int32_t)(int8_t)bytes[0];
        case 2: return (int32_t)((uint16_t)(bytes[0] | bytes[1] << 8));
        case 4: return (int32_t)((uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24 );
        default: return 0;
    }
}

static inline int32_t get_disp(Instruction *instr)
{
    return little_endian_to_int32(instr->displacement, instr->displacement_length);
}
 
// only for special cases where displacement becomes the absolute address (i.e. mod=00 and rm=101 with no SIB)
static inline uint32_t get_unsigned_disp(Instruction *instr)
{
    return little_endian_to_uint32(instr->displacement, instr->displacement_length);
}

static inline uint32_t get_unsigned_imm(Instruction *instr)
{
    return little_endian_to_uint32(instr->immediate, instr->immediate_length);
}

static inline int32_t get_signed_imm(Instruction *instr)
{
    return little_endian_to_int32(instr->immediate, instr->immediate_length);
}


static inline char get_operand_size(CPU *cpu, BUS *bus, Instruction *instr)
{
    /*
    If L (long mode) is set for a code segment descriptor, D must be clear. 
    The L=1 / D=1 combination is currently meaningless / reserved. 
    If L is clear, then D selects between 16 and 32-bit mode.
     (i.e. the default operand / address size).
    */
    // operand size is determined by the D-bit in the Code Segment
        // if D=0, default operand is 16 bits
        // if D=1, default operand is 32 bits
        // Unless there exists operand-size override prefix

    // the D flag is bit 22 of the CS descriptor
    uint64_t descriptor = get_descriptor(bus, &cpu->gdtr, get_SegmentRegister_index(&cpu->segment_registers[CS]));
    // default bit
    char d_bit = (descriptor >> 22) & 1;

    return (instr->has_operand_size_override) ? !d_bit : d_bit;
}

static inline int seg_bounds_check(CPU *cpu, uint32_t new_addr, SegmentRegisterType seg_type)
{
    bool valid = new_addr < (cpu->segment_registers[seg_type].limit + cpu->segment_registers[seg_type].base);
    if (!valid)
    {
        if (seg_type == CS)
            cpu->halt = 1;
        return FAILED_SEGBOUND_CHECK;
    } // need to raise a #GP fault, abort execution
    return EXECUTE_SUCCESS;
}
//=======================================================================================================================================================
//========================================================Effective Address Calculation Helpers==========================================================

static inline int bus_read_checked(BUS *bus, uint32_t *data, uint32_t addr, size_t width)
{
    int bus_result = bus_read(bus, data, addr, width);
    return (bus_result == EXECUTE_SUCCESS) ? EXECUTE_SUCCESS : BUS_READ_FAILURE;
}

static inline int bus_write_checked(BUS *bus, uint32_t data, uint32_t addr, size_t width)
{
    int bus_result = bus_write(bus, data, addr, width);
    return (bus_result == EXECUTE_SUCCESS) ? EXECUTE_SUCCESS : BUS_WRITE_FAILURE;
}

static int calculate_EA(CPU *cpu, Instruction *instr, uint32_t *effective_addr_out)
{

    if (instr->mod == 0x3) 
        return INVALID_EFFECTIVE_ADDRESS;
    if (instr->sib_length == 0 && instr->mod == 0x00 && instr->rm_field == 0x05) 
    {
        if (instr->displacement_length != 4)
            return INVALID_EFFECTIVE_ADDRESS; // the disp needs to be an absoluate address
        *effective_addr_out = get_unsigned_disp(instr); // disp32 only with no SIB. In this case, displacement is an address, not an offset
        return EXECUTE_SUCCESS;
    }
        
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
        else
        {
            *effective_addr_out = effective_addr; // disp32 only WITH SIB
            return EXECUTE_SUCCESS;
        }
    }

    effective_addr += base; 
    *effective_addr_out = effective_addr;
    return EXECUTE_SUCCESS;
}

//=======================================================================================================================================================
//===============================================================OPERAND READERS=================================================================================

static int read_rm_op(BUS *bus, CPU *cpu, Instruction *decoded_instr, size_t width, uint32_t *value_out)
{
    if(decoded_instr->mod == 0x3) // register direct
    {
        *value_out = gpr_w_handler(cpu, decoded_instr->rm_field, width);
        return EXECUTE_SUCCESS;
    }

    uint32_t effective_addr = 0;
    int result = calculate_EA(cpu, decoded_instr, &effective_addr);
    if (result != EXECUTE_SUCCESS)
        return result;

    result = bus_read_checked(bus, value_out, effective_addr, width);
    if (result != EXECUTE_SUCCESS)
        return result;

    return EXECUTE_SUCCESS;
}

static int write_rm_dst(BUS *bus, CPU *cpu, Instruction *decoded_instr, size_t width, uint32_t result, uint32_t *effective_addr_out)
{
    if(decoded_instr->mod == 0x3)
        set_gpr_w_handler(cpu, decoded_instr->rm_field, width, result);
    else
    {
        uint32_t effective_addr = 0;
        int result = calculate_EA(cpu, decoded_instr, &effective_addr);
        if (result != EXECUTE_SUCCESS)
            return result;

        result = bus_write_checked(bus, result, effective_addr, width);
        if (result != EXECUTE_SUCCESS)
            return result;

        if (effective_addr_out)
            *effective_addr_out = effective_addr;
    }
    return EXECUTE_SUCCESS;

}



//=======================================================================================================================================================
//===============================================================ARITHMETIC AND LOGIC HELPERS=================================================================================

static int alu_two_op_rm_r(BUS *bus, CPU *cpu, Instruction *decoded_instr, size_t width, Opclass opclass, int cin)
{

    ALU_out out = { .low = 0, .high = 0, .flags_out = 0, .cin = 0 };
    uint32_t op1 = 0;
    int result = read_rm_op(bus, cpu, decoded_instr, width, &op1);
    if (result != EXECUTE_SUCCESS)
        return result;
    uint32_t op2 = gpr_w_handler(cpu, decoded_instr->reg_or_opcode, width);
    ALU(op1, op2, cin, width, opclass, &out);

    if (!(opclass & OPC_THROW_RESULT_MASK))
    {
        result = write_rm_dst(bus, cpu, decoded_instr, width, out.low, NULL);
        if (result != EXECUTE_SUCCESS)
            return result;
    }
    update_status_register(cpu, opclass, out.flags_out);

    return EXECUTE_SUCCESS;
}

static int alu_two_op_r_rm(BUS *bus, CPU *cpu, Instruction *decoded_instr, size_t width, Opclass opclass, int cin)
{
    ALU_out out = { .low = 0, .high = 0, .flags_out = 0, .cin = 0 };                                                        
    uint32_t op1 = gpr_w_handler(cpu, decoded_instr->reg_or_opcode, width); 
    uint32_t op2 = 0;
    int result = read_rm_op(bus, cpu, decoded_instr, width, &op2);
    if (result != EXECUTE_SUCCESS)
        return result;
    ALU(op1, op2, cin, width, opclass, &out);

    if (!(opclass & OPC_THROW_RESULT_MASK))
        set_gpr_w_handler(cpu, decoded_instr->reg_or_opcode, width, out.low);
    update_status_register(cpu, opclass, out.flags_out);

    return EXECUTE_SUCCESS;                                                                                                       
}

//=======================================================================================================================================================
//===============================================================00 - 0F=================================================================================

int execute_ADD_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 8, OPC_ADD, NO_CIN);
}

int execute_ADD_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 32, OPC_ADD, NO_CIN);
}

int execute_ADD_R8_RM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 8, OPC_ADD, NO_CIN);
}

int execute_ADD_R32_RM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 32, OPC_ADD, NO_CIN);
}

int execute_ADD_AL_IMM8 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return UNIMPLEMENTED_INSTRUCTION; }
int execute_ADD_EAX_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return UNIMPLEMENTED_INSTRUCTION; }

// For the segment register ES
int execute_PUSH_ES(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return UNIMPLEMENTED_INSTRUCTION; }
int execute_POP_ES(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return UNIMPLEMENTED_INSTRUCTION; }


int execute_OR_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 8, OPC_OR, NO_CIN);
}

int execute_OR_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 32, OPC_OR, NO_CIN);
}

int execute_OR_R8_RM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 8, OPC_OR, NO_CIN);
}

int execute_OR_R32_RM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 32, OPC_OR, NO_CIN);
}


//=======================================================================================================================================================
//===============================================================10 - 1F=================================================================================

int execute_ADC_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    
    uint32_t cin = (cpu->status_register >> 2) & 0x1;
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 8, OPC_ADC, cin);
}

int execute_ADC_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    uint32_t cin = (cpu->status_register >> 2) & 0x1;
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 32, OPC_ADC, cin);
}

int execute_ADC_R8_RM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    uint32_t cin = (cpu->status_register >> 2) & 0x1;
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 8, OPC_ADC, cin);
}

int execute_ADC_R32_RM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    uint32_t cin = (cpu->status_register >> 2) & 0x1;
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 32, OPC_ADC, cin);
}


// For the segment register SS
int execute_PUSH_SS(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return UNIMPLEMENTED_INSTRUCTION; }
int execute_POP_SS(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return UNIMPLEMENTED_INSTRUCTION; }

int execute_SBB_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    uint32_t cin = (cpu->status_register >> 2) & 0x1;
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 8, OPC_SBB, cin);
}

int execute_SBB_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    uint32_t cin = (cpu->status_register >> 2) & 0x1;
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 32, OPC_SBB, cin);
}

int execute_SBB_R8_RM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    uint32_t cin = (cpu->status_register >> 2) & 0x1;
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 8, OPC_SBB, cin);
}

int execute_SBB_R32_RM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    uint32_t cin = (cpu->status_register >> 2) & 0x1;
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 32, OPC_SBB, cin);
}


//=======================================================================================================================================================
//===============================================================20 - 2F=================================================================================
int execute_AND_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 8, OPC_AND, NO_CIN);
}

int execute_AND_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 32, OPC_AND, NO_CIN);
}

int execute_AND_R8_RM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 8, OPC_AND, NO_CIN);
}

int execute_AND_R32_RM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 32, OPC_AND, NO_CIN);
}


int execute_AND_AL_IMM8   (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }
int execute_AND_EAX_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }
int execute_SEG_ES        (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }
int execute_DAA           (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }

int execute_SUB_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 8, OPC_SUB, NO_CIN);
}

int execute_SUB_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 32, OPC_SUB, NO_CIN);
}

int execute_SUB_R8_RM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 8, OPC_SUB, NO_CIN);
}

int execute_SUB_R32_RM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 32, OPC_SUB, NO_CIN);
}


//=======================================================================================================================================================
//===============================================================30 - 3F=================================================================================

int execute_XOR_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 8, OPC_XOR, NO_CIN);
}

int execute_XOR_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 32, OPC_XOR, NO_CIN);
}

int execute_XOR_R8_RM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 8, OPC_XOR, NO_CIN);
}

int execute_XOR_R32_RM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 32, OPC_XOR, NO_CIN);
}


int execute_XOR_AL_IMM8    (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }
int execute_XOR_EAX_IMM32  (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }

int execute_CMP_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 8, OPC_CMP, NO_CIN);
}

int execute_CMP_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 32, OPC_CMP, NO_CIN);
}

int execute_CMP_R8_RM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 8, OPC_CMP, NO_CIN);
}

int execute_CMP_R32_RM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 32, OPC_CMP, NO_CIN);
}


int execute_CMP_AL_IMM8   (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }
int execute_CMP_EAX_IMM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }


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
    return 1;

}

int execute_INC_EAX(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_increment_register(bus, cpu, decoded_instr);
}

int execute_INC_ECX(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_increment_register(bus, cpu, decoded_instr);
}

int execute_INC_EDX(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_increment_register(bus, cpu, decoded_instr);
}

int execute_INC_EBX(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_increment_register(bus, cpu, decoded_instr);
}

int execute_INC_ESP(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_increment_register(bus, cpu, decoded_instr);
}

int execute_INC_EBP(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    //printw("===================EXECUTING INC_EBP======================\n");
    return alu_increment_register(bus, cpu, decoded_instr);
}

int execute_INC_ESI(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    //printw("===================EXECUTING INC_ESI======================\n");
    return alu_increment_register(bus, cpu, decoded_instr);
}

int execute_INC_EDI(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    //printw("===================EXECUTING INC_EDI======================\n");
    return alu_increment_register(bus, cpu, decoded_instr);
}


//=======================================================================================================================================================
//===============================================================50 - 5F=================================================================================

// first decrements ESP by four, then pushes contents at address
// loads first byte of DWORD at [SS:ESP], then second byte at [SS:ESP] + 1, etc...
static int push_gpr(BUS *bus, CPU *cpu, Instruction *decoded_instr, GeneralPurposeRegisterType gpr)
{
    set_gpr32(cpu, ESP, gpr32(cpu, ESP) - 4);
    int result = bus_write_checked(bus, gpr32(cpu, gpr), address_translator(cpu, SS, ESP), 32);
    if (result != EXECUTE_SUCCESS)
    {
        set_gpr32(cpu, ESP, gpr32(cpu, ESP) + 4);
        return result;
    }
    return EXECUTE_SUCCESS;
}

// first transfers the dword at the current top of stack to destination operand
// increments ESP to point to new top of stack
static int pop_gpr(BUS *bus, CPU *cpu, Instruction *decoded_instr, GeneralPurposeRegisterType gpr)
{
    uint32_t popped_data;
    int result = bus_read_checked(bus, &popped_data, address_translator(cpu, SS, ESP), 32);
    if (result != EXECUTE_SUCCESS) { return result; }
    set_gpr32(cpu, gpr, popped_data);
    result = bus_write_checked(bus, 0, address_translator(cpu, SS, ESP), 32);
    if (result != EXECUTE_SUCCESS) { return result; } // clear stack
    set_gpr32(cpu, ESP, gpr32(cpu, ESP) + 4);
    return EXECUTE_SUCCESS;
}

int execute_PUSH_EAX(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    //printw("===================EXECUTING PUSH_EAX======================\n");
    return push_gpr(bus, cpu, decoded_instr, EAX);
} 

int execute_PUSH_ECX(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("===================EXECUTING PUSH_ECX======================\n");
    return push_gpr(bus, cpu, decoded_instr, ECX);
} 

int execute_PUSH_EDX(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("===================EXECUTING PUSH_EDX======================\n");
    return push_gpr(bus, cpu, decoded_instr, EDX);
} 

int execute_PUSH_EBX(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("===================EXECUTING PUSH_EBX======================\n");
    return push_gpr(bus, cpu, decoded_instr, EBX);
} 

int execute_PUSH_ESP(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("===================EXECUTING PUSH_ESP======================\n");
    return push_gpr(bus, cpu, decoded_instr, ESP);
} 

int execute_PUSH_EBP(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("===================EXECUTING PUSH_EBP======================\n");
    return push_gpr(bus, cpu, decoded_instr, EBP);
} 

int execute_PUSH_ESI(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("===================EXECUTING PUSH_ESI======================\n");
    return push_gpr(bus, cpu, decoded_instr, ESI);
} 

int execute_PUSH_EDI(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("===================EXECUTING PUSH_EDI======================\n");
    return push_gpr(bus, cpu, decoded_instr, EDI);
} 

int execute_POP_EAX(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    //printw("===================EXECUTING POP_EAX======================\n");
    return pop_gpr(bus, cpu, decoded_instr, EAX);
} 

int execute_POP_ECX(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("===================EXECUTING POP_ECX======================\n");
    return pop_gpr(bus, cpu, decoded_instr, ECX);
} 

int execute_POP_EDX(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("===================EXECUTING POP_EDX======================\n");
    return pop_gpr(bus, cpu, decoded_instr, EDX);
} 

int execute_POP_EBX(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("===================EXECUTING POP_EBX======================\n");
    return pop_gpr(bus, cpu, decoded_instr, EBX);
} 

int execute_POP_ESP(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("===================EXECUTING POP_ESP======================\n");
    return pop_gpr(bus, cpu, decoded_instr, ESP);
} 

int execute_POP_EBP(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("===================EXECUTING POP_EBP======================\n");
    return pop_gpr(bus, cpu, decoded_instr, EBP);
} 

int execute_POP_ESI(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("===================EXECUTING POP_ESI======================\n");
    return pop_gpr(bus, cpu, decoded_instr, ESI);
} 

int execute_POP_EDI(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("===================EXECUTING POP_EDI======================\n");
    return pop_gpr(bus, cpu, decoded_instr, EDI);
} 


//=======================================================================================================================================================
//===============================================================60 - 6F=================================================================================

int execute_PUSHA(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("===================EXECUTING PUSHA======================\n");
    int result = execute_PUSH_EAX(bus, cpu, decoded_instr);
    if (result != EXECUTE_SUCCESS) { return result; }
    result = execute_PUSH_ECX(bus, cpu, decoded_instr);
    if (result != EXECUTE_SUCCESS) { return result; }
    result = execute_PUSH_EDX(bus, cpu, decoded_instr);
    if (result != EXECUTE_SUCCESS) { return result; }
    result = execute_PUSH_EBX(bus, cpu, decoded_instr);
    if (result != EXECUTE_SUCCESS) { return result; }
    result = execute_PUSH_ESP(bus, cpu, decoded_instr);
    if (result != EXECUTE_SUCCESS) { return result; }
    result = execute_PUSH_EBP(bus, cpu, decoded_instr);
    if (result != EXECUTE_SUCCESS) { return result; }
    result = execute_PUSH_ESI(bus, cpu, decoded_instr);
    if (result != EXECUTE_SUCCESS) { return result; }
    result = execute_PUSH_EDI(bus, cpu, decoded_instr);
    if (result != EXECUTE_SUCCESS) { return result; }
    return EXECUTE_SUCCESS;
}

int execute_POPA(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("===================EXECUTING POPA======================\n");
    int result = execute_POP_EAX(bus, cpu, decoded_instr);
    if (result != EXECUTE_SUCCESS) { return result; }
    result = execute_POP_ECX(bus, cpu, decoded_instr);
    if (result != EXECUTE_SUCCESS) { return result; }
    result = execute_POP_EDX(bus, cpu, decoded_instr);
    if (result != EXECUTE_SUCCESS) { return result; }
    result = execute_POP_EBX(bus, cpu, decoded_instr);
    if (result != EXECUTE_SUCCESS) { return result; }
    result = execute_POP_ESP(bus, cpu, decoded_instr);
    if (result != EXECUTE_SUCCESS) { return result; }
    result = execute_POP_EBP(bus, cpu, decoded_instr);
    if (result != EXECUTE_SUCCESS) { return result; }
    result = execute_POP_ESI(bus, cpu, decoded_instr);
    if (result != EXECUTE_SUCCESS) { return result; }
    result = execute_POP_EDI(bus, cpu, decoded_instr);
    if (result != EXECUTE_SUCCESS) { return result; }
    return EXECUTE_SUCCESS;
}


// first decrements ESP by four, then pushes immediate at address
// loads first byte of DWORD at [SS:ESP], then second byte at [SS:ESP] + 1, etc...
static int push_imm32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    set_gpr32(cpu, ESP, gpr32(cpu, ESP) - 4);
    int result = bus_write_checked(bus, get_unsigned_imm(decoded_instr), address_translator(cpu, SS, ESP), 32);
    if (result != EXECUTE_SUCCESS)
    {
        set_gpr32(cpu, ESP, gpr32(cpu, ESP) + 4);
        return result;
    }
    return EXECUTE_SUCCESS;
}

static int push_imm8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    set_gpr32(cpu, ESP, gpr32(cpu, ESP) - 1);
    int result = bus_write_checked(bus, 0xFF & get_unsigned_imm(decoded_instr), address_translator(cpu, SS, ESP), 8);
    if (result != EXECUTE_SUCCESS)
    {
        set_gpr32(cpu, ESP, gpr32(cpu, ESP) + 1);
        return result;
    }
    return EXECUTE_SUCCESS;
}

//0x68
int execute_PUSH_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    //printw("===================EXECUTING PUSH_IMM32======================\n");
    return push_imm32(bus, cpu, decoded_instr);
}

//0x6A
int execute_PUSH_IMM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    //printw("===================EXECUTING PUSH_IMM8======================\n");
    return push_imm8(bus, cpu, decoded_instr);
}

int execute_BOUND_GV_MA(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }

//=======================================================================================================================================================
//===============================================================70 - 7F=================================================================================

int jmp_rel32(CPU *cpu, Instruction *decoded_instr) 
{
    int32_t imm = get_signed_imm(decoded_instr);
    set_gpr32(cpu, EIP, gpr32(cpu, EIP) + imm);
    // bounds check, if wrong, rollback and raise error
    uint32_t new_addr = address_translator(cpu, CS, EIP);

    int result = seg_bounds_check(cpu, new_addr, CS);
    if (result != EXECUTE_SUCCESS)
    {
        set_gpr32(cpu, EIP, gpr32(cpu, EIP) - imm);
        cpu->halt = true;
        return result;
    }
    return EXECUTE_SUCCESS;
}

int jmp_rel8(CPU *cpu, Instruction *decoded_instr) 
{
    // uses imm length from decoded_instr to handle sign extension
    // 8-bit imm is sign-extended to 32 bits
    int32_t imm = get_signed_imm(decoded_instr);
    set_gpr32(cpu, EIP, gpr32(cpu, EIP) + imm);
    uint32_t new_addr = address_translator(cpu, CS, EIP);

    int result = seg_bounds_check(cpu, new_addr, CS);
    if (result != EXECUTE_SUCCESS)
    {
        set_gpr32(cpu, EIP, gpr32(cpu, EIP) - imm);
        cpu->halt = true;
        return result;
    }
    return EXECUTE_SUCCESS;
}

// 70 cb JO rel8 7+m,3 Jump short if overflow (OF=1)
int execute_JO_REL8(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    bool overflow = (cpu->status_register & OF) != 0;
    if (overflow)
        return jmp_rel8(cpu, decoded_instr);
    return EXECUTE_SUCCESS;
}

// 71 cb JNO rel8 7+m,3 Jump short if not overflow (OF=0)
int execute_JNO_REL8(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    bool overflow = (cpu->status_register & OF) != 0;
    if (!overflow)
        return jmp_rel8(cpu, decoded_instr);
    return EXECUTE_SUCCESS;
}

// 72 cb JB rel8 7+m,3 Jump short if below (CF=1)
// 72 cb JC rel8 7+m,3 Jump short if carry (CF=1)
// 72 cb JNAE rel8 7+m,3 Jump short if not above or equal (CF=1)
int execute_JB_REL8(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    bool carry = (cpu->status_register & CF) != 0;
    if (carry)
        return jmp_rel8(cpu, decoded_instr);
    return EXECUTE_SUCCESS;
}

// 73 cb JNB rel8 7+m,3 Jump short if not below (CF=0)
// 73 cb JNC rel8 7+m,3 Jump short if not carry (CF=0)
// 73 cb JAE rel8 7+m,3 Jump short if above or equal (CF=0)
int execute_JAE_REL8(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    bool carry = (cpu->status_register & CF) != 0;
    if (!carry)
        return jmp_rel8(cpu, decoded_instr);
    return EXECUTE_SUCCESS;
}

// 74 cb JE rel8 7+m,3 Jump short if equal (ZF=1)
// 74 cb JZ rel8 7+m,3 Jump short if 0 (ZF=1)
// 74 cb JZ rel8 7+m,3 Jump short if zero (ZF = 1)
int execute_JE_REL8(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    bool zero = (cpu->status_register & ZF) != 0;
    if (zero)
        return jmp_rel8(cpu, decoded_instr);
    return EXECUTE_SUCCESS;
}

// 75 cb JNE rel8 7+m,3 Jump short if not equal (ZF=0)
// 75 cb JNZ rel8 7+m,3 Jump short if not zero (ZF=0)
int execute_JNE_REL8(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    bool zero = (cpu->status_register & ZF) != 0;
    if (!zero)
        return jmp_rel8(cpu, decoded_instr);
    return EXECUTE_SUCCESS;
}

// 76 cb JBE rel8 7+m,3 Jump short if below or (CF=1 or ZF=1)
// 76 cb JNA rel8 7+m,3 Jump short if not above (CF=1 ZF=1)
int execute_JBE_REL8(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    bool carry = (cpu->status_register & CF) != 0;
    bool zero = (cpu->status_register & ZF) != 0;
    if (carry || zero)
        return jmp_rel8(cpu, decoded_instr);
    return EXECUTE_SUCCESS;
}

// 77 cb JNBE rel8 7+m,3 Jump short if not below or equal (CF=0 and ZF=0)
// 77 cb JA rel8 7+m,3 Jump short if above (CF=0 and ZF=0)
int execute_JA_REL8(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    bool carry = (cpu->status_register & CF) != 0;
    bool zero = (cpu->status_register & ZF) != 0;
    if (!carry && !zero)
        return jmp_rel8(cpu, decoded_instr);
    return EXECUTE_SUCCESS;
}

// 78 cb JS rel8 7+m,3 Jump short if sign (SF=1)
int execute_JS_REL8(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    bool sign = (cpu->status_register & SF) != 0;
    if (sign)
        return jmp_rel8(cpu, decoded_instr);
    return EXECUTE_SUCCESS;
}
// 79 cb JNS rel8 7+m,3 Jump short if not sign (SF=0)
int execute_JNS_REL8(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    bool sign = (cpu->status_register & SF) != 0;
    if (!sign)
        return jmp_rel8(cpu, decoded_instr);
    return EXECUTE_SUCCESS;
}

// 7A cb JPE rel8 7+m,3 Jump short if parity even (PF=1)
// 7A cb JP rel8 7+m,3 Jump short if parity (PF=1)
int execute_JP_REL8(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    bool parity = (cpu->status_register & PF) != 0;
    if (parity)
        return jmp_rel8(cpu, decoded_instr);
    return EXECUTE_SUCCESS;
}

// 7B cb JPO rel8 7+m,3 Jump short if parity odd (PF=0)
// 7B cb JNP rel8 7+m,3 Jump short if not parity (PF=0)
int execute_JNP_REL8(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    bool parity = (cpu->status_register & PF) != 0;
    if (!parity)
        return jmp_rel8(cpu, decoded_instr);
    return EXECUTE_SUCCESS;
}

// 7C cb JL rel8 7+m,3 Jump short if less (SF≠OF)
// 7C cb JNGE rel8 7+m,3 Jump short if not greater or equal (SF≠OF)
int execute_JL_REL8(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    bool is_less = (cpu->status_register & SF) != (cpu->status_register & OF);
    if (is_less)
        return jmp_rel8(cpu, decoded_instr);
    return EXECUTE_SUCCESS;
}

// 7D cb JGE rel8 7+m,3 Jump short if greater or equal (SF=OF)
// 7D cb JNL rel8 7+m,3 Jump short if not less (SF=OF)
int execute_JGE_REL8(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    bool is_greater_equal = (cpu->status_register & SF) == (cpu->status_register & OF);
    if (is_greater_equal)
        return jmp_rel8(cpu, decoded_instr);
    return EXECUTE_SUCCESS;
}

// 7E cb JLE rel8 7+m,3 Jump short if less or equal (ZF=1 and SF≠OF)
// 7E cb JNG rel8 7+m,3 Jump short if not greater (ZF=1 or SF≠OF)
int execute_JLE_REL8(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    bool less_or_equal = ((cpu->status_register & ZF) != 0) && ((cpu->status_register & SF) != (cpu->status_register & OF));
    if (less_or_equal)
        return jmp_rel8(cpu, decoded_instr);
    return EXECUTE_SUCCESS;
}
// 7F cb JG rel8 7+m,3 Jump short if greater (ZF=0 and SF=OF)
// 7F cb JNLE rel8 7+m,3 Jump short if not less or equal (ZF=0 and SF=OF)
int execute_JG_REL8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    bool greater = ((cpu->status_register & ZF) == 0) && ((cpu->status_register & SF) == (cpu->status_register & OF));
    if (greater)
        return jmp_rel8(cpu, decoded_instr);
    return EXECUTE_SUCCESS;
} 

//=======================================================================================================================================================
//===============================================================80 - 8F=================================================================================

int execute_IMM_GRP_EB_LB   (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }  
int execute_IMM_GRP_EV_LZ   (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }  
int execute_IMM_GRP_EV_LB   (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }  

int execute_TEST_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 8, OPC_TEST, NO_CIN);
}


int execute_TEST_RM32_R32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    //printw("===================EXECUTING TEST_RM32_R32======================\n");
    return alu_two_op_rm_r(bus, cpu, decoded_instr, 32, OPC_TEST, NO_CIN);
}

int execute_TEST_R8_RM8(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    //printw("===================EXECUTING TEST_R8_RM8======================\n");
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 8, OPC_TEST, NO_CIN);
}

int execute_TEST_R32_RM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    //printw("===================EXECUTING TEST_R32_RM32======================\n");
    return alu_two_op_r_rm(bus, cpu, decoded_instr, 32, OPC_TEST, NO_CIN);
}

int execute_MOV_RM8_R8(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("\n========Executing MOV_RM8_R8...============\n");
    uint32_t reg_src_value = gpr_w_handler(cpu, decoded_instr->reg_or_opcode, 8);

    if (decoded_instr->mod == 0x3) {
        // r/m is  a register, write to it
        set_gpr_w_handler(cpu, decoded_instr->rm_field, 8, reg_src_value);
    } else {
        // r/m is memory, write to it
        uint32_t effective_addr = 0;
        int result = calculate_EA(cpu, decoded_instr, &effective_addr);
        if (result != EXECUTE_SUCCESS) { return result; }
        result = bus_write_checked(bus, reg_src_value, effective_addr, 8);
        if (result != EXECUTE_SUCCESS) { return result; }
        print_cell((uint8_t)reg_src_value, effective_addr);
    }
    //printw("===================EXECUTE DONE======================\n");
    return EXECUTE_SUCCESS;
}

int execute_MOV_RM32_R32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("\n========Executing MOV_RM32_R32...============\n");
    uint32_t reg_src_value = gpr_w_handler(cpu, decoded_instr->reg_or_opcode, 32);

    if (decoded_instr->mod == 0x3) {
        // r/m is  a register, write to it
        set_gpr_w_handler(cpu, decoded_instr->rm_field, 32, reg_src_value);
    } else {
        // r/m is memory, write to it
        uint32_t effective_addr = 0;
        int result = calculate_EA(cpu, decoded_instr, &effective_addr);
        if (result != EXECUTE_SUCCESS) { return result; }
        result = bus_write_checked(bus, reg_src_value, effective_addr, 32);
        if (result != EXECUTE_SUCCESS) { return result; }
        print_dword(reg_src_value, effective_addr);
    }
    //printw("===================EXECUTE DONE======================\n");
    return EXECUTE_SUCCESS;
}

int execute_MOV_R8_RM8 (BUS *bus, CPU *cpu, Instruction *decoded_instr)    
{
    //printw("\n========Executing MOV_R8_RM8...============\n");

    if (decoded_instr->mod == 0x3) 
    {
        set_gpr_w_handler(cpu, decoded_instr->reg_or_opcode, 8, gpr_w_handler(cpu, decoded_instr->rm_field, 8));
        //printw("Value %x moved to register\n", gpr_w_handler(cpu, decoded_instr->rm_field, 8));
    }

    else 
    {
        uint32_t from_mem_value = 0;
        uint32_t effective_addr = 0;
        int result = calculate_EA(cpu, decoded_instr, &effective_addr);
        if (result != EXECUTE_SUCCESS) { return result; }
        result = bus_read_checked(bus, &from_mem_value, effective_addr, 8);
        if (result != EXECUTE_SUCCESS) { return result; }
        set_gpr_w_handler(cpu, decoded_instr->reg_or_opcode, 8, from_mem_value);
        //printw("Value %x moved to register\n", from_mem_value);
    }
    return EXECUTE_SUCCESS;
}

int execute_MOV_R32_RM32 (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    //printw("\n========Executing MOV_R32_RM32...============\n");

    if (decoded_instr->mod == 0x3) 
    {
        set_gpr_w_handler(cpu, decoded_instr->reg_or_opcode, 32, gpr_w_handler(cpu, decoded_instr->rm_field, 32));
        //printw("Value %x moved to register\n", gpr_w_handler(cpu, decoded_instr->rm_field, 32));
    }

    else 
    {
        uint32_t from_mem_value = 0;
        uint32_t effective_addr = 0;
        int result = calculate_EA(cpu, decoded_instr, &effective_addr);
        if (result != EXECUTE_SUCCESS) { return result; }
        result = bus_read_checked(bus, &from_mem_value, effective_addr, 32);
        if (result != EXECUTE_SUCCESS) { return result; }
        set_gpr_w_handler(cpu, decoded_instr->reg_or_opcode, 32, from_mem_value);
        //printw("Value %x moved to register\n", from_mem_value);
    }
    return EXECUTE_SUCCESS;
}

int execute_LEA(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void)bus; (void)cpu; (void)decoded_instr;return UNIMPLEMENTED_INSTRUCTION; }

//=======================================================================================================================================================
//===============================================================90 - 9F=================================================================================


//=======================================================================================================================================================
//===============================================================A0 - AF=================================================================================

int execute_MOV_AL_MOFFS8   (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }  
int execute_MOV_EAXv_MOFFSv (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }  
int execute_MOV_MOFFS8_AL   (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }  
int execute_MOV_MOFFSv_EAXv (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }  

//=======================================================================================================================================================
//===============================================================B0 - BF=================================================================================

int execute_MOV_AL_IMM8     (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { set_gpr_w_handler(cpu, AL, 8, get_unsigned_imm(decoded_instr)); return EXECUTE_SUCCESS; }  
int execute_MOV_CL_IMM8     (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { set_gpr_w_handler(cpu, CL, 8, get_unsigned_imm(decoded_instr)); return EXECUTE_SUCCESS; }  
int execute_MOV_DL_IMM8     (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { set_gpr_w_handler(cpu, DL, 8, get_unsigned_imm(decoded_instr)); return EXECUTE_SUCCESS; }  
int execute_MOV_BL_IMM8     (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { set_gpr_w_handler(cpu, BL, 8, get_unsigned_imm(decoded_instr)); return EXECUTE_SUCCESS; }  
int execute_MOV_AH_IMM8     (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { set_gpr_w_handler(cpu, AH, 8, get_unsigned_imm(decoded_instr)); return EXECUTE_SUCCESS; }  
int execute_MOV_CH_IMM8     (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { set_gpr_w_handler(cpu, CH, 8, get_unsigned_imm(decoded_instr)); return EXECUTE_SUCCESS; }  
int execute_MOV_DH_IMM8     (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { set_gpr_w_handler(cpu, DH, 8, get_unsigned_imm(decoded_instr)); return EXECUTE_SUCCESS; }  
int execute_MOV_BH_IMM8     (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { set_gpr_w_handler(cpu, BH, 8, get_unsigned_imm(decoded_instr)); return EXECUTE_SUCCESS; }  

int execute_MOV_R8_IMM8     (BUS *bus, CPU *cpu, Instruction *decoded_instr)   { set_gpr_w_handler(cpu, AL, 8, get_unsigned_imm(decoded_instr)); return EXECUTE_SUCCESS; }  

int execute_MOV_EAX_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    //printw("===================EXECUTING MOV_EAX_IMM32======================\n");
    (void)bus;
    set_gpr_w_handler(cpu, EAX, 32, get_unsigned_imm(decoded_instr));
    return 1;
}

int execute_MOV_ECX_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    //printw("===================EXECUTING MOV_ECX_IMM32======================\n");
    (void)bus;
    set_gpr_w_handler(cpu, ECX, 32, get_unsigned_imm(decoded_instr));
    return 1;
}

int execute_MOV_EDX_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    //printw("===================EXECUTING MOV_EDX_IMM32======================\n");
    (void)bus;
    set_gpr_w_handler(cpu, EDX, 32, get_unsigned_imm(decoded_instr));
    return 1;
}

int execute_MOV_EBX_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    //printw("===================EXECUTING MOV_EBX_IMM32======================\n");
    (void)bus;
    set_gpr_w_handler(cpu, EBX, 32, get_unsigned_imm(decoded_instr));
    return 1;
}

int execute_MOV_ESP_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    //printw("===================EXECUTING MOV_ESP_IMM32======================\n");
    (void)bus;
    set_gpr_w_handler(cpu, ESP, 32, get_unsigned_imm(decoded_instr));
    return 1;
}

int execute_MOV_EBP_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    //printw("===================EXECUTING MOV_EBP_IMM32======================\n");
    (void)bus;
    set_gpr_w_handler(cpu, EBP, 32, get_unsigned_imm(decoded_instr));
    return 1;
}

int execute_MOV_ESI_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    //printw("===================EXECUTING MOV_ESI_IMM32======================\n");
    (void)bus;
    set_gpr_w_handler(cpu, ESI, 32, get_unsigned_imm(decoded_instr));
    return 1;
}

int execute_MOV_EDI_IMM32(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    //printw("===================EXECUTING MOV_EDI_IMM32======================\n");
    (void)bus;
    set_gpr_w_handler(cpu, EDI, 32, get_unsigned_imm(decoded_instr));
    return 1;
}


//=======================================================================================================================================================
//===============================================================C0 - CF=================================================================================

int execute_GRP2_EB_IB(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }
int execute_GRP2_EV_IB(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }

// C2: return near, pop imm16 bytes of parameters
int execute_RET_NEAR_IMM16(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }
// C3: return (near) to caller
// take 32-bit return address from the top of the stack as SS:ESP
// load that value into EIP
// advance ESP by the size of pointer
int execute_RET_NEAR(BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{  
    return pop_gpr(bus, cpu, decoded_instr, EIP);
}

int execute_LES(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }
int execute_LDS(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }
int execute_MOV_EB_IB(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }
int execute_MOV_EV_IV(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }
int execute_ENTER(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }
int execute_LEAVE(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }

// CA
int execute_RET_FAR_IMM16(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }
// CB return (far) to caller
int execute_RET_FAR(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }

int execute_INT3(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }
int execute_INT_IB(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }
int execute_INTO(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }
int execute_IRET(BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }

//=======================================================================================================================================================
//===============================================================D0 - DF=================================================================================

int execute_SHIFT_EB_1   (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }    
int execute_SHIFT_EV_1   (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }    
int execute_SHIFT_EB_CL  (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }    
int execute_SHIFT_EV_CL  (BUS *bus, CPU *cpu, Instruction *decoded_instr) { (void) bus; (void)cpu; (void)decoded_instr; return UNIMPLEMENTED_INSTRUCTION; }    

//=======================================================================================================================================================
//===============================================================E0 - EF=================================================================================

// push the address of instruction following CALL onto the stack, then 
// jump to the address at EIP + signed 32-bit displacement
int execute_CALL_REL32    (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    // the EIP before executing instructions already point to the next instruction
    int result = push_gpr(bus, cpu, decoded_instr, EIP);
    if (result != EXECUTE_SUCCESS)
        return result;
    return jmp_rel32(cpu, decoded_instr);
}

/*
Near jump—A jump to an instruction within the current code segment (the segment currently pointed to by the CS register), sometimes referred to as an intrasegment jump.
Short jump—A near jump where the jump range is limited to –128 to +127 from the current EIP value.
*/

// Relative jumps
// E9: Jump near, EIP = EIP + 32 bit displacement
int execute_JMP_REL32     (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    return jmp_rel32(cpu, decoded_instr);
}

// EB: Jump short, EIP = EIP + 8 bit displacement sign-extended to 32 bits
int execute_JMP_REL8      (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    return jmp_rel8(cpu, decoded_instr);
}

//=======================================================================================================================================================
//===============================================================F0 - FF=================================================================================

int execute_CLC (BUS *bus, CPU *cpu, Instruction *decoded_instr) //F9
{
    cpu->status_register &= ~(CF);
    return 1;
}

int execute_STC (BUS *bus, CPU *cpu, Instruction *decoded_instr) //F9
{
    cpu->status_register |= CF;
    return 1;
}


int execute_HLT (BUS *bus, CPU *cpu, Instruction *decoded_instr) 
{
    (void)bus;
    (void)decoded_instr;
    // raise HLT flag
    cpu->halt = 1;
    return 1;
}

/*

// operand size is determined by the D-bit in the Code Segment
    // if D=0, default operand is 16 bits
    // if D=1, default operand is 32 bits
    // Unless there exists operand-size override prefix
Group 5, determined by bits 5-3 of the ModR/M byte:
FF /0: INC rm16, rm32
FF /1: DEC rm16, rm32
FF /2: CALL rm16, rm32 (near, absolute indirect)
FF /3: CALL m16:16, m16:32 (far, absolute indirect)
FF /4: JMP rm16, rm32 (near, absolute indirect) 
FF /5: JMP m16:16, m16:32 (far, absolute indirect)
FF /6: PUSH rm16, rm32
*/

//CALL (Call Procedure) activates an out-of-line procedure, saving on the
//stack the address of the instruction following the CALL for later use by a
//RET (Return) instruction. CALL places the current value of EIP on the stack.
//The RET instruction in the called procedure uses this address to transfer
//control back to the calling program.
//CALL instructions, like JMP instructions have relative, direct, and
//indirect versions.
//Indirect CALL instructions specify an absolute address in one of these
//ways:
// 1. The program can CALL a location specified by a general register (any
// of EAX, EDX, ECX, EBX, EBP, ESI, or EDI). The processor moves this
// 32-bit value into EIP.
// 2. The processor can obtain the destination address from a memory
// operand specified in the instruction.

int execute_GRP5(BUS *bus, CPU *cpu, Instruction *decoded_instr)
{
    uint32_t rm_value = 0;
    int result = read_rm_op(bus, cpu, decoded_instr, 32, &rm_value);
    if (result != EXECUTE_SUCCESS)
        return result;

    ALU_out out = { .low = 0, .high = 0, .flags_out = 0, .cin = 0 };
    switch(decoded_instr->reg_or_opcode)
    {
        case 0: // INC rm32
            ALU(/*op1, base*/rm_value,
                /*op2, none*/0,
                            NO_CIN, 32, OPC_INC, &out);
            result = write_rm_dst(bus, cpu, decoded_instr, 32, out.low, NULL);
            if (result != EXECUTE_SUCCESS)
                return result;
            update_status_register(cpu, OPC_INC, out.flags_out);
            return EXECUTE_SUCCESS;

        case 1: // DEC rm32
            ALU(/*op1, base*/rm_value,
                /*op2, none*/0,
                            NO_CIN, 32, OPC_DEC, &out);
            result = write_rm_dst(bus, cpu, decoded_instr, 32, out.low, NULL);
            if (result != EXECUTE_SUCCESS)
                return result;
            update_status_register(cpu, OPC_DEC, out.flags_out);
            return EXECUTE_SUCCESS;

        case 2: // CALL rm32
            // push the address of instruction following CALL onto the stack,
            // THEN jump to the address in rm32
            result = push_gpr(bus, cpu, decoded_instr, EIP);
            if (result != EXECUTE_SUCCESS)
                return result;
            set_gpr32(cpu, EIP, rm_value);
            return EXECUTE_SUCCESS;

        case 4: // JMP rm32
        {
            uint32_t new_addr = cpu->segment_registers[CS].base + rm_value;
            result = seg_bounds_check(cpu, new_addr, CS);
            if (result != EXECUTE_SUCCESS)
                return result;
            (get_operand_size(cpu, bus, decoded_instr)) ? set_gpr32(cpu, EIP, rm_value) : set_gpr16(cpu, EIP, rm_value & 0xFFFF);
            return EXECUTE_SUCCESS;
        }

        default:
            return UNIMPLEMENTED_INSTRUCTION;
    }
}
