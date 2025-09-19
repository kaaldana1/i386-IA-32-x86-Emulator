
#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "ids/opcode_list.h"
#include "core/structs/operand_addr_form.h"

typedef struct {
    uint8_t prefix[4];
    uint8_t prefix_length;
    bool has_operand_size_override;
    bool has_addr_size_override;
    bool has_rep;
    bool has_lock;
    bool has_segment_override;
    bool has_CS;
    bool has_SS;
    bool has_DS;
    bool has_ES;
    bool has_FS;
    bool has_GS;

    uint8_t opcode[3];
    uint8_t opcode_length;
    Opcode_ID opcode_id;

    uint8_t modrm;
    uint8_t modrm_length;
    uint8_t mod, reg_or_opcode, rm_field;
    Operand_addr_form operands;

    uint8_t sib;
    uint8_t scale, index, base;
    uint8_t sib_length;

    uint8_t displacement[4];
    uint8_t displacement_length;

    uint8_t immediate[4];
    uint8_t immediate_length;

    uint8_t total_length;
} Instruction;

#endif
