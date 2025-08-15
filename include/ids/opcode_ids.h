#ifndef OPCODE_IDS_H
#define OPCODE_IDS_H

#include "ids/opcode_list.h"

typedef enum
{
#define X(name, _1, _2, _3, _4, _5) name,
    FOREACH_OPCODE(X)
#undef X
OPCODE_COUNT
} Opcode_ID;

#endif
