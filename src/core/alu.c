#include "core/alu.h"
#include "ids/opclass_list.h"
#include "ids/register_ids.h"

#define ODD_PARITY 1
#define EVEN_PARITY 0


#define FORWARD_DECLARE_ALU_FUNC(op, _value, _expr) static int alu_##op(uint32_t, uint32_t, int, size_t, ALU_out *);
FOR_EACH_ARITH_LOGIC_OP(FORWARD_DECLARE_ALU_FUNC)
#undef FORWARD_DECLARE_ALU_FUNC


static inline void set_SF(ALU_out *out) {  out->flags_out |= SF;  }
static inline void set_ZF(ALU_out *out) {  out->flags_out |= ZF;  }
static inline void set_CF(ALU_out *out) {  out->flags_out |= CF;  }
static inline void set_AF(ALU_out *out) {  out->flags_out |= AF;  }
static inline void set_PF(ALU_out *out) {  out->flags_out |= PF;  }
static inline void set_OF(ALU_out *out) {  out->flags_out |= OF;  }
static inline void set_DF(ALU_out *out) {  out->flags_out |= DF;  }
static inline void set_IF(ALU_out *out) {  out->flags_out |= IF;  }
static inline void set_TF(ALU_out *out) {  out->flags_out |= TF;  }

static inline uint32_t op_mask(uint32_t op, size_t width)
{
    return (width == 32) ? op : (op & ((1 << width) - 1));
}

//=========================================STATUS FLAG FUNCTIONS=============================
/* Flag ── Set on high-order bit carry or borrow; cleared
 otherwise.*/
static inline bool is_carry(uint32_t result, size_t width) 
{
    return (result >> width) & 0x1;
}

static inline bool is_borrow(uint32_t op1, uint32_t op2) 
{
    return (op1 < op2);
}

/*  Adjust flag ── Set on carry from or borrow to the low order
 four bits of AL; cleared otherwise. Used for decimal
 arithmetic. */
static inline bool is_nibble_overflow(uint32_t op1, uint32_t op2) 
{
    return ((op1 + op2) >> 4) & 0x1;
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
 static inline bool is_overflow(uint32_t op1, uint32_t op2, uint32_t result)
 {
    if (op1 > 0 && op2 > 0 && result < 0) return true; // positive overflow
    if (op1 < 0 && op2 < 0 && result > 0) return true; // negative overflow
    return false;
 }

/* Set if low-order eight bits of result contain
 an even number of 1 bits; cleared otherwise */
static inline bool check_parity(uint32_t x, size_t width) 
{
    //check parity of lsb
    uint8_t high = ((uint8_t)x >> 4) & 0xF;
    uint8_t low = (uint8_t)x & 0xF;

    return high ^ low;
} 

static inline void update_potential_flags(uint32_t op1, uint32_t op2, ALU_out *out, size_t width)
{
    if (is_carry(out->low, width)) 
        set_CF(out);

    if (is_borrow(out->low, width)) 
        set_CF(out);

    if (out->low == 0) 
        set_ZF(out);

    if(is_signed(out->low, width))
        set_SF(out);

    if(is_overflow(op1, op2, out->low))
        set_OF(out);

    if(!check_parity(out->low, width))
        set_PF(out);

    if(is_nibble_overflow(op1, op2))
        set_AF(out);
}


//========================================EXECUTE FUNCTIONS==============================================
#define ALU_FUNC_BODIES(op, _value, expr) \
static inline int alu_##op(uint32_t op1, uint32_t op2, int cin, size_t width, ALU_out *out) \
{ \
    printf("+=====================+\n"); \
    printf("|      ALU ACCESS     |  \n"); \
    printf("|      op:  %s       |  \n", #op); \
    printf("+=====================+\n"); \
    uint32_t result = expr ;\
    out->low = result; \
    update_potential_flags(op1, op2, out, width); \
    return 1; \
} 
FOR_EACH_ARITH_LOGIC_OP(ALU_FUNC_BODIES)
#undef ALU_FUNC_BODIES


int ALU(uint32_t op1, uint32_t op2, int cin,
        size_t width,  Opclass opclass, ALU_out *out)
{
    switch(opclass) 
    {
        #define ALU_FUNC_CASE(op, _value, _expr) \
        case OPC_##op: return alu_##op(op1, op2, cin, width, out); \
        break; 
        FOR_EACH_ARITH_LOGIC_OP(ALU_FUNC_CASE)
        #undef ALU_FUNC_CASE
    }

}