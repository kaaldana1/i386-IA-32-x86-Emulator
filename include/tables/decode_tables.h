#ifndef DECODE_TABLES_H
#define DECODE_TABLES_H

#include "core/structs/instruction_metadata.h"
#include "core/structs/instruction.h"
#include "ids/register_ids.h"

#define MODRM_MOD_COUNT 4
#define MODRM_REG_COUNT 8
#define MODRM_RM_COUNT 8

extern const Operand_addr_form operand_addr_form_lut[MODRM_MOD_COUNT][MODRM_RM_COUNT][MODRM_REG_COUNT]; 
extern const Instruction_metadata instr_metadata_lut[OPCODE_COUNT];
extern const Opcode_ID single_byte_opcode_lut[OPCODE_COUNT];

#endif
