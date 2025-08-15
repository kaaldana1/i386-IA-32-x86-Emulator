#include "tables/decode_tables.h"
#include <stdbool.h>

#define REG_TO_REG 0
#define REG_TO_IMM 1
#define REG_TO_MEM 2
#define MEM_TO_REG 3
#define IMM_TO_REG 4

#define HAS_MODRM true
#define NO_IMMEDIATE 0
#define SINGLE_BYTE_IMM 1
#define FOUR_BYTE_IMM 4
#define NO_MODRM false
#define PLACEHOLDER 0 
//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================
// these are indexed by REG and RM values
extern const Operand_addr_form operand_addr_form_lut[MODRM_MOD_COUNT][MODRM_RM_COUNT][MODRM_REG_COUNT] = { 
    {
     {{EAX, EAX}, {EAX, ECX}, {EAX, EDX}, {EAX, EBX}, {EAX, ESP}, {EAX, EBP}, {EAX, ESI}, {EAX, EDI} },
     {{ECX, EAX}, {ECX, ECX}, {ECX, EDX}, {ECX, EBX}, {ECX, ESP}, {ECX, EBP}, {ECX, ESI}, {ECX, EDI} },
     {{EDX, EAX}, {EDX, ECX}, {EDX, EDX}, {EDX, EBX}, {EDX, ESP}, {EDX, EBP}, {EDX, ESI}, {EDX, EDI} },
     {{EBX, EAX}, {EBX, ECX}, {EBX, EDX}, {EBX, EBX}, {EBX, ESP}, {EBX, EBP}, {EBX, ESI}, {EBX, EDI} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
    },

    {
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} }
    }, 

    {
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} }
    }, 

    {
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} },
     {{PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER}, {PLACEHOLDER, PLACEHOLDER} }
    }, 
}; // will need to change this

//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================

extern const Instruction_metadata instr_metadata_lut[OPCODE_COUNT] = { // also indexed by opcode id
#define X(_name, _op_byte, has_modrm, imm_bytes, operand_count, operand_type) {has_modrm, imm_bytes, operand_count, operand_type},
    FOREACH_OPCODE(X)
#undef X
};

//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================

extern const Opcode_ID single_byte_opcode_lut[256] = {
    // this should be expanded to 256 entries once completed (some will be invalid)
    #define X(name, opcode_byte, _1, _2, _3, _4) [opcode_byte] = name,
        FOREACH_OPCODE(X)
    #undef X
};
