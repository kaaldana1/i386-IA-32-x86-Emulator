#include "core/alu.h"
#include "ids/opclass_list.h"
#include "ids/register_ids.h"

#define GENERATE_CARRY_MASK (OPC_ADD | OPC_ADC | OPC_INC)
#define GENERATE_BORROW_MASK (OPC_SUB | OPC_SBB | OPC_CMP | OPC_DEC )

static inline void set_SF(ALU_out *out) {   out->flags_out |= SF;  }
static inline void set_ZF(ALU_out *out) {   out->flags_out |= ZF;  }
static inline void set_CF_carry(ALU_out *out) {   out->flags_out |= CF;  }
static inline void set_AF(ALU_out *out) {   out->flags_out |= AF;  }
static inline void set_PF(ALU_out *out) {   out->flags_out |= PF;  }
static inline void set_OF(ALU_out *out) {   out->flags_out |= OF;  }
static inline void set_DF(ALU_out *out) {   out->flags_out |= DF;  }
static inline void set_IF(ALU_out *out) {   out->flags_out |= IF;  }
static inline void set_TF(ALU_out *out) {   out->flags_out |= TF;  }
static inline void set_CF_borrow(ALU_out *out) {   out->flags_out |= (1 << 9);  }


static inline uint32_t op_mask(uint32_t op, size_t width)
{
    return (width == 32) ? op : (op & ((1 << width) - 1));
}

//=========================================STATUS FLAG FUNCTIONS=============================
/* Flag ── Set on high-order bit carry or borrow; cleared
 otherwise.*/
static inline bool is_carry(ALU_out *out, size_t width) 
{
    if (width != 32)
        return (out->low >> width) & 0x1;
    else 
        return (out->cin);
}

static inline bool is_borrow(uint32_t op1, uint32_t op2, int cin) 
{
    if (op1 < (op2 + cin)) return 1;
    return 0;
}

/*  Adjust flag ── Set on carry from or borrow to the low order
 four bits of AL; cleared otherwise. Used for decimal
 arithmetic. */
static inline bool is_nibble_overflow(Opclass opclass, uint32_t op1, uint32_t op2, int cin) 
{
    bool af = 0;
    if (opclass & GENERATE_CARRY_MASK)
        af = (((op1 & 0x0F) + (op2 & 0x0F) + cin) > 0xF);
    if (opclass & GENERATE_BORROW_MASK)
        af = ((op1 & 0x0F)  < (op2 & 0x0F));
    return af;
}

/* Sign Flag ── Set equal to high-order bit of result (0 is
 positive, 1 if negative).*/
static inline bool is_signed(uint32_t result, size_t width)
{
    return (result >> (width - 1)) & 0x1;
}

/*OF Overflow Flag ── Set if result is too large a positive number
 or too small a negative number (excluding sign-bit) to fit in
 destination operand; cleared otherwise.*/
 /*overflow occurs when operands have the same sign but result in a different sign*/
 static inline bool is_overflow(uint32_t op1, uint32_t op2, uint32_t result)
 {
    uint32_t sign_op1 = op1 ^ result;
    uint32_t sign_op2 = op2 ^ result;
    return (sign_op1 & sign_op2 & 0x80000000u) != 0;
 }

/* Set if low-order eight bits of result contain
 an even number of 1 bits; cleared otherwise */
static inline bool check_parity(uint32_t x) 
{
    //check parity of lsb
    uint8_t high = ((uint8_t)x >> 4) & 0xF;
    uint8_t low = (uint8_t)x & 0xF;
    uint8_t res = high ^ low;

    high = (res >> 2) & 0x3;
    low = res & 0x3;
    res = high ^ low;

    high = (res >> 1) & 0x1;
    low = res & 0x1;

    return high ^ low;
} 

static inline void update_potential_flags(Opclass opclass, uint32_t op1, uint32_t op2, int cin, ALU_out *out, size_t width)
{
    if (is_carry(out, width)) 
        set_CF_carry(out);

    if (is_borrow(op1, op2, cin)) 
        set_CF_borrow(out);

    if (out->low == 0) 
        set_ZF(out);

    if(is_signed(out->low, width))
        set_SF(out);

    if(is_overflow(op1, op2, out->low))
        set_OF(out);

    if(((out->low & 0xFF) != 0) && (check_parity(out->low) == 0))
        set_PF(out);

    if(is_nibble_overflow(opclass, op1, op2, cin))
        set_AF(out);
}

// static void alu_print(uint32_t op1, uint32_t op2, int cin, ALU_out *out, const char *op)
// {
    /*
    //printw("+=====================+\n");            
    //printw("|      ALU ACCESS     |  \n");          
    //printw("|      op:  %s       |  \n", op);      
    //printw("+=====================+\n");            
    //printw("+============================+\n"); 
    //printw("|      op1: %08X        |  \n", op1); 
    //printw("|      op2: %08X        |  \n", op2); 
    //printw("|      cin: %08X        |  \n", cin); 
    //printw("|      result: %08X     |  \n", out->low); 
    //printw("|       flags: %08x     |\n",   out->flags_out); 
    //printw("+============================+\n"); 
    */
//}


//========================================EXECUTE FUNCTIONS==============================================
static int alu_body (Opclass opclass, uint32_t op1, uint32_t op2, int cin, size_t width, ALU_out *out, const char *name, 
                     void (*expr)(uint32_t op1, uint32_t op2, int cin, ALU_out *out))
{
    op1 = op_mask(op1, width);
    op2 = op_mask(op2, width);
    expr(op1, op2, cin, out);
    update_potential_flags(opclass, op1, op2, cin, out, width);
    // alu_print(op1, op2, cin, out, name);
    return 1; 
}

static inline void expr_ADD(uint32_t op1, uint32_t op2, int cin, ALU_out *out) 
{  
    uint64_t result = ((uint64_t)op1 + (uint64_t)op2);
    out->low = (uint32_t)result;
    out->cin = (result >> 32) & 0x1;
}      

static inline void expr_SUB(uint32_t op1, uint32_t op2, int cin, ALU_out *out)     
{  
    uint64_t result = (op1 + ((~op2)  + 1));
    out->low = (uint32_t)result;
    out->cin = (result >> 32) & 0x1;
}      

static inline void expr_CMP(uint32_t op1, uint32_t op2, int cin, ALU_out *out) 
{  
    uint64_t result = (op1 + ((~op2)  + 1));
    out->low = (uint32_t)result;
    out->cin = (result >> 32) & 0x1;
}      

static inline void expr_NEG(uint32_t op1, uint32_t op2, int cin, ALU_out *out) { out->low = -op1; }
static inline void expr_ADC(uint32_t op1, uint32_t op2, int cin, ALU_out *out) 
{ 
    uint64_t result = ((uint64_t)op1 + (uint64_t)op2 + (uint64_t)cin); 
    out->low = (uint32_t)result;
    out->cin = (result >> 32) & 0x1;
}      

static inline void expr_SBB(uint32_t op1, uint32_t op2, int cin, ALU_out *out )       
{  
    uint64_t result = (op1 + ((~op2)  + 1));
    result += ((~(uint32_t)cin) + 1);
    out->low = (uint32_t)result;
    out->cin = (result >> 32) & 0x1;
}      

static inline void expr_INC(uint32_t op1, uint32_t op2, int cin, ALU_out *out)      
{  
    uint64_t result = ((uint64_t)op1 + 1);
    out->low = (uint32_t)result;
    out->cin = (result >> 32) & 0x1;
}      

static inline void expr_DEC(uint32_t op1, uint32_t op2, int cin, ALU_out *out)      
{  
    uint64_t result = ((uint64_t)op1 -1 );
    out->low = (uint32_t)result;
    out->cin = (result >> 32) & 0x1;
}      

static inline void expr_AND(uint32_t op1, uint32_t op2, int cin, ALU_out *out)  {  out->low =  (op1 & op2); }   
static inline void expr_TEST(uint32_t op1, uint32_t op2, int cin, ALU_out *out)  {  out->low =  (op1 & op2); }   
static inline void expr_XOR(uint32_t op1, uint32_t op2, int cin, ALU_out *out)  {  out->low =  (op1 ^ op2); }   
static inline void expr_OR (uint32_t op1, uint32_t op2, int cin, ALU_out *out)  {  out->low =  (op1 | op2); }    
static inline void expr_SHL(uint32_t op1, uint32_t op2, int cin, ALU_out *out) { out->low = (op1 << 1); }   
static inline void expr_SHR(uint32_t op1, uint32_t op2, int cin, ALU_out *out) { out->low = (op1 >> 1); }   
static inline void expr_SAR(uint32_t op1, uint32_t op2, int cin, ALU_out *out) { out->low = (op1 >> 1); }   
static inline void expr_ROL(uint32_t op1, uint32_t op2, int cin, ALU_out *out) { out->low = (op1 << 1); }   
static inline void expr_ROR(uint32_t op1, uint32_t op2, int cin, ALU_out *out) { out->low = (op1 >> 1); }  

#define ALU_FUNCS(op, _value)                                                                           \
static int alu_##op(Opclass opclass, uint32_t op1, uint32_t op2, int cin, size_t width, ALU_out *out)   \
{                                                                                                       \
    return alu_body(opclass, op1, op2, cin, width, out, #op, expr_##op );                               \
} 
FOR_EACH_ARITH_LOGIC_OP(ALU_FUNCS)
#undef ALU_FUNCS


int ALU(uint32_t op1, uint32_t op2, int cin,
        size_t width,  Opclass opclass, ALU_out *out)
{
    switch(opclass) 
    {
        #define ALU_FUNC_CASE(op, _value)                                                              \
        case OPC_##op: return alu_##op(opclass, op1, op2, cin, width, out);                            \
        break; 
        FOR_EACH_ARITH_LOGIC_OP(ALU_FUNC_CASE)
        #undef ALU_FUNC_CASE
        default:
            return 0;
    }

}