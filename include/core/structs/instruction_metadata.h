#ifndef INSTRUCTION_METADATA_H
#define INSTRUCTION_METADATA_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct
{
    bool has_modrm;
    size_t immediate_bytes;
    size_t operand_count;
    int operand_type;
} Instruction_metadata;

#endif
