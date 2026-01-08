#include "ids/str_opcode_list.h"
#include "ids/opcode_list.h"

const char* string_opcode_list[256] = { 
    #define OPCODE(name, opcode_byte, _1, _2, _3, _4, _5, _6) [opcode_byte] = #name, 
    FOREACH_OPCODE(OPCODE)  
    #undef OPCODE
};