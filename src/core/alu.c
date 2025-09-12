#include "core/alu.h"

#define ODD_PARITY 1
#define EVEN_PARITY 0

static inline void set_SF(ALU_out *out) {  out->flags_out |= SF;  }
static inline void set_ZF(ALU_out *out) {  out->flags_out |= ZF;  }
static inline void set_CF(ALU_out *out) {  out->flags_out |= CF;  }
static inline void set_AF(ALU_out *out) {  out->flags_out |= AF;  }
static inline void set_PF(ALU_out *out) {  out->flags_out |= PF;  }
static inline void set_OF(ALU_out *out) {  out->flags_out |= OF;  }
static inline void set_DF(ALU_out *out) {  out->flags_out |= DF;  }
static inline void set_IF(ALU_out *out) {  out->flags_out |= IF;  }
static inline void set_TF(ALU_out *out) {  out->flags_out |= TF;  }

/*static inline bool check_parity(uint32_t x, size_t width) 
{
    size_t bit_shift = width / 2;
    uint32_t high = (x >> (bit_shift));
    uint32_t low = x & ((1 << (bit_shift)) - 1);
    uint32_t res = 0;

    for(int i = 0; i <= log2(width); i++)
    {
        res = high ^ low;
        bit_shift = bit_shift / 2;
        high = res >> bit_shift;
        low = res & ((1 << (bit_shift)) - 1);
    }

    return res; // res is 1 if odd parity
} */

static inline bool check_parity(uint32_t x, size_t width) 
{
    //check parity of lsb
    uint8_t high = ((uint8_t)x >> 4) & 0xF;
    uint8_t low = (uint8_t)x & 0xF;

    return high ^ low;
} 

static inline uint32_t op_mask(uint32_t op, size_t width)
{
    return (width == 32) ? op : (op & ((1 << width) - 1));
}

static inline uint16_t update_flag_ADD(ALU_out *out, size_t width)
{
        if (!(out->low >= 0 && out->low < 255)) // change this
        {
            set_CF(out);
        } 

        if (out->low == 0) 
        {
            set_ZF(out);
        }

        if(out->low < 0)
        {
            set_SF(out);
        }

        /*if()
        {
            set_OF(out);
        }*/

        if(!check_parity(out->low, width))
        {
            set_PF(out);
        }

        if(out->low = 2 /*check auxiliary flag*/)
        {
            set_AF(out);
        }
}



static inline void alu_ADD(uint32_t op1, uint32_t op2, size_t width, ALU_out *out)
{
    uint32_t result = (op1 + op2) & ((1 << width) - 1);
    out->low = result;
    update_flag_ADD(out, width);
}

int ALU(uint32_t op1, uint32_t op2, bool cin,
        size_t width,  Opclass opclass, ALU_out *out)
{
    switch(opclass) 
    {
        case ADD: 
            alu_ADD(op1, op2, width, out);
            return 1;
        break;

        case ADC:
        break;

        case SUB:
        break;
        
        case SBB:
        break;

        case CMP:
        break;

        case INC:
        break;

        case DEC:
        break;
        
        case NEG:
        break;

        case AND:
        break;

        case XOR:
        break;

        case OR:
        break;

        case TEST:
        break;
    }
}