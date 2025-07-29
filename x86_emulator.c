#include "x86_emulator.h"
extern uint8_t ram_16kb[RAM_SIZE];
extern gdt_entry global_desc_table[FLAT_MODE_GDT_SIZE];
extern int initialize_ram(Program *program);


int interpreter()
{
    int start_addr;
    uint16_t CS_index = get_seg_reg_index(&segment_registers[CS]);
    uint32_t CS_limit = get_gdt_limit(&global_desc_table[CS_index]);
    uint32_t CS_base = get_gdt_base(&global_desc_table[CS_index]);

    while (gen_purpose_registers[EIP].dword < (CS_base + CS_limit) && !halt_flag)
    {
        start_addr = gen_purpose_registers[EIP].dword;

        uint8_t instr_buff[MAX_INSTR_LENGTH];
        memset(instr_buff, 0x00, sizeof(instr_buff));

        // Assume MAX instruction length, and truncate accordingly
        memcpy(instr_buff, (ram_16kb + start_addr), 15);
        Instruction *decoded_instruction = decoder(instr_buff);
        gen_purpose_registers[EIP].byte[0] += decoded_instruction->total_length;

#ifdef DEBUG
        printf("\nTotal instruction length: %hu\n", decoded_instruction->total_length);
#endif

        if ((*execution_handler_lut[decoded_instruction->opcode_id])(decoded_instruction))
        {
            printf("\nExecution complete\n");
        }
    }

    return 1;
}

int initialize(Program *program)
{
    parse_file(program);
    create_gdt();
    initialize_ram(program);
    initialize_segment_registers();
    initialize_gen_purpose_registers();

    print_registers();

    return 1;
}

int main()
{
    Program test;
    if (initialize(&test))
    {
        print_contents(&test);
    }

    if (interpreter())
    {
        printf("\nyay\n");
    }

    return 0;
}