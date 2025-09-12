#ifndef INSTRUCTION_METADATA_H
#define INSTRUCTION_METADATA_H

#include <stdlib.h>
#include <stdbool.h>
#include "ids/opclass.h"

typedef struct
{
    bool has_modrm;
    size_t immediate_bytes;
    size_t operand_count;
    int operand_type;
    Opclass opclass;
    size_t width;

} Instruction_metadata;

#endif
