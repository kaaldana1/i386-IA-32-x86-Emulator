#include "cpu.h"

int initialize_gen_purpose_registers() {
    uint16_t CS_index = seg_reg_index(&segment_registers[CS]);
    uint32_t CS_base = gdt_base(&global_desc_table[CS_index]);

    gen_purpose_registers[EIP].dword = CS_base;
    gen_purpose_registers[ESP].dword = HIGHEST_RAM_ADDRESS;
    return 0;
}

int initialize_segment_registers() {
    segment_registers[CS].word = 0x0018;
    segment_registers[SS].word = 0x0020;
    return 0;
}
