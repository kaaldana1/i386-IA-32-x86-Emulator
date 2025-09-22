#ifndef DECODE_TABLES_H
#define DECODE_TABLES_H

#include "core/structs/instruction.h"
#include "core/structs/InstructionMetadata.h"
#include "ids/register_ids.h"


extern const InstructionMetadata instr_metadata_lut[256];
extern const OpcodeID single_byte_opcode_lut[256];

#endif
