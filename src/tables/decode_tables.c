#include "tables/decode_tables.h"
#include <stdbool.h>


const InstructionMetadata instr_metadata_lut[256] = { // also indexed by opcode id
#define X(_name, _op_byte, has_modrm, imm_bytes, operand_count, OperandType, opclass, width) {has_modrm, imm_bytes, operand_count, OperandType, opclass, width},
    FOREACH_OPCODE(X)
#undef X
};

//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================

const OpcodeID single_byte_opcode_lut[256] = {
    // this should be expanded to 256 entries once completed (some will be invalid)
    #define X(name, opcode_byte, _1, _2, _3, _4, _5, _6) [opcode_byte] = name,
        FOREACH_OPCODE(X)
    #undef X
};
