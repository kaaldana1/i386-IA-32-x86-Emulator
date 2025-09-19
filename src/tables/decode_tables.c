#include "tables/decode_tables.h"
#include <stdbool.h>

//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================
// these are indexed by REG and RM values

// Indexed as [mod][r/m][reg] --> { effective_addr_register, src_register }
extern const Operand_addr_form operand_addr_form_lut[MODRM_MOD_COUNT][MODRM_RM_COUNT][MODRM_REG_COUNT] = {

// ============================ MOD = 00 ============================ 
{
  /* r/m = 000 */ { {EAX, EAX}, {EAX, ECX}, {EAX, EDX}, {EAX, EBX}, {EAX, ESP}, {EAX, EBP}, {EAX, ESI}, {EAX, EDI} },
  /* r/m = 001 */ { {ECX, EAX}, {ECX, ECX}, {ECX, EDX}, {ECX, EBX}, {ECX, ESP}, {ECX, EBP}, {ECX, ESI}, {ECX, EDI} },
  /* r/m = 010 */ { {EDX, EAX}, {EDX, ECX}, {EDX, EDX}, {EDX, EBX}, {EDX, ESP}, {EDX, EBP}, {EDX, ESI}, {EDX, EDI} },
  /* r/m = 011 */ { {EBX, EAX}, {EBX, ECX}, {EBX, EDX}, {EBX, EBX}, {EBX, ESP}, {EBX, EBP}, {EBX, ESI}, {EBX, EDI} },
  /* r/m = 100 */ { {PLACEHOLDER, EAX}, {PLACEHOLDER, ECX}, {PLACEHOLDER, EDX}, {PLACEHOLDER, EBX},
                      {PLACEHOLDER, ESP}, {PLACEHOLDER, EBP}, {PLACEHOLDER, ESI}, {PLACEHOLDER, EDI} },
  /* r/m = 101 */ { {PLACEHOLDER, EAX}, {PLACEHOLDER, ECX}, {PLACEHOLDER, EDX}, {PLACEHOLDER, EBX},
                      {PLACEHOLDER, ESP}, {PLACEHOLDER, EBP}, {PLACEHOLDER, ESI}, {PLACEHOLDER, EDI} },
  /* r/m = 110 */ { {ESI, EAX}, {ESI, ECX}, {ESI, EDX}, {ESI, EBX}, {ESI, ESP}, {ESI, EBP}, {ESI, ESI}, {ESI, EDI} },
  /* r/m = 111 */ { {EDI, EAX}, {EDI, ECX}, {EDI, EDX}, {EDI, EBX}, {EDI, ESP}, {EDI, EBP}, {EDI, ESI}, {EDI, EDI} }
},

// ============================ MOD = 01 ============================ 
{
  /* r/m = 000 */ { {EAX, EAX}, {EAX, ECX}, {EAX, EDX}, {EAX, EBX}, {EAX, ESP}, {EAX, EBP}, {EAX, ESI}, {EAX, EDI} },
  /* r/m = 001 */ { {ECX, EAX}, {ECX, ECX}, {ECX, EDX}, {ECX, EBX}, {ECX, ESP}, {ECX, EBP}, {ECX, ESI}, {ECX, EDI} },
  /* r/m = 010 */ { {EDX, EAX}, {EDX, ECX}, {EDX, EDX}, {EDX, EBX}, {EDX, ESP}, {EDX, EBP}, {EDX, ESI}, {EDX, EDI} },
  /* r/m = 011 */ { {EBX, EAX}, {EBX, ECX}, {EBX, EDX}, {EBX, EBX}, {EBX, ESP}, {EBX, EBP}, {EBX, ESI}, {EBX, EDI} },
  /* r/m = 100 */ { {PLACEHOLDER, EAX}, {PLACEHOLDER, ECX}, {PLACEHOLDER, EDX}, {PLACEHOLDER, EBX},
                              {PLACEHOLDER, ESP}, {PLACEHOLDER, EBP}, {PLACEHOLDER, ESI}, {PLACEHOLDER, EDI} },
  /* r/m = 101 */ { {EBP, EAX}, {EBP, ECX}, {EBP, EDX}, {EBP, EBX}, {EBP, ESP}, {EBP, EBP}, {EBP, ESI}, {EBP, EDI} },
  /* r/m = 110 */ { {ESI, EAX}, {ESI, ECX}, {ESI, EDX}, {ESI, EBX}, {ESI, ESP}, {ESI, EBP}, {ESI, ESI}, {ESI, EDI} },
  /* r/m = 111 */ { {EDI, EAX}, {EDI, ECX}, {EDI, EDX}, {EDI, EBX}, {EDI, ESP}, {EDI, EBP}, {EDI, ESI}, {EDI, EDI} }
},

// ============================ MOD = 10 ============================ 
{
  /* r/m = 000 */ { {EAX, EAX}, {EAX, ECX}, {EAX, EDX}, {EAX, EBX}, {EAX, ESP}, {EAX, EBP}, {EAX, ESI}, {EAX, EDI} },
  /* r/m = 001 */ { {ECX, EAX}, {ECX, ECX}, {ECX, EDX}, {ECX, EBX}, {ECX, ESP}, {ECX, EBP}, {ECX, ESI}, {ECX, EDI} },
  /* r/m = 010 */ { {EDX, EAX}, {EDX, ECX}, {EDX, EDX}, {EDX, EBX}, {EDX, ESP}, {EDX, EBP}, {EDX, ESI}, {EDX, EDI} },
  /* r/m = 011 */ { {EBX, EAX}, {EBX, ECX}, {EBX, EDX}, {EBX, EBX}, {EBX, ESP}, {EBX, EBP}, {EBX, ESI}, {EBX, EDI} },
  /* r/m = 100 */ { {PLACEHOLDER, EAX}, {PLACEHOLDER, ECX}, {PLACEHOLDER, EDX}, {PLACEHOLDER, EBX},
                               {PLACEHOLDER, ESP}, {PLACEHOLDER, EBP}, {PLACEHOLDER, ESI}, {PLACEHOLDER, EDI} },
  /* r/m = 101 */ { {EBP, EAX}, {EBP, ECX}, {EBP, EDX}, {EBP, EBX}, {EBP, ESP}, {EBP, EBP}, {EBP, ESI}, {EBP, EDI} },
  /* r/m = 110 */ { {ESI, EAX}, {ESI, ECX}, {ESI, EDX}, {ESI, EBX}, {ESI, ESP}, {ESI, EBP}, {ESI, ESI}, {ESI, EDI} },
  /* r/m = 111 */ { {EDI, EAX}, {EDI, ECX}, {EDI, EDX}, {EDI, EBX}, {EDI, ESP}, {EDI, EBP}, {EDI, ESI}, {EDI, EDI} }
},

// ============================ MOD = 11 ============================ 
{
  /* r/m = 000 */ { {EAX, EAX}, {EAX, ECX}, {EAX, EDX}, {EAX, EBX}, {EAX, ESP}, {EAX, EBP}, {EAX, ESI}, {EAX, EDI} },
  /* r/m = 001 */ { {ECX, EAX}, {ECX, ECX}, {ECX, EDX}, {ECX, EBX}, {ECX, ESP}, {ECX, EBP}, {ECX, ESI}, {ECX, EDI} },
  /* r/m = 010 */ { {EDX, EAX}, {EDX, ECX}, {EDX, EDX}, {EDX, EBX}, {EDX, ESP}, {EDX, EBP}, {EDX, ESI}, {EDX, EDI} },
  /* r/m = 011 */ { {EBX, EAX}, {EBX, ECX}, {EBX, EDX}, {EBX, EBX}, {EBX, ESP}, {EBX, EBP}, {EBX, ESI}, {EBX, EDI} },
  /* r/m = 100 */ { {ESP, EAX}, {ESP, ECX}, {ESP, EDX}, {ESP, EBX}, {ESP, ESP}, {ESP, EBP}, {ESP, ESI}, {ESP, EDI} },
  /* r/m = 101 */ { {EBP, EAX}, {EBP, ECX}, {EBP, EDX}, {EBP, EBX}, {EBP, ESP}, {EBP, EBP}, {EBP, ESI}, {EBP, EDI} },
  /* r/m = 110 */ { {ESI, EAX}, {ESI, ECX}, {ESI, EDX}, {ESI, EBX}, {ESI, ESP}, {ESI, EBP}, {ESI, ESI}, {ESI, EDI} },
  /* r/m = 111 */ { {EDI, EAX}, {EDI, ECX}, {EDI, EDX}, {EDI, EBX}, {EDI, ESP}, {EDI, EBP}, {EDI, ESI}, {EDI, EDI} }
}

};

//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================

const Instruction_metadata instr_metadata_lut[256] = { // also indexed by opcode id
#define X(_name, _op_byte, has_modrm, imm_bytes, operand_count, operand_type, opclass, width) {has_modrm, imm_bytes, operand_count, operand_type, opclass, width},
    FOREACH_OPCODE(X)
#undef X
};

//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================

const Opcode_ID single_byte_opcode_lut[256] = {
    // this should be expanded to 256 entries once completed (some will be invalid)
    #define X(name, opcode_byte, _1, _2, _3, _4, _5, _6) [opcode_byte] = name,
        FOREACH_OPCODE(X)
    #undef X
};
