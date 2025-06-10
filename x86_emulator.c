#include "x86_emulator.h"
extern uint8_t ram_16kb[RAM_SIZE];
extern int initialize_ram(Program *program);

int interpreter() {
    int start_addr;
    while (registers[EIP].dword < (uint32_t)ram_struct.text_size - 1){
        start_addr = registers[EIP].dword;

        uint8_t instr_buff[MAX_INSTR_LENGTH];
        memset(instr_buff, 0x00, sizeof(instr_buff));

        // Assume MAX instruction length, and truncate accordingly
        memcpy(instr_buff, ((ram_16kb + ram_struct.text_base) + start_addr), 15);
        Instruction *decoded_instruction = decoder(instr_buff);
        registers[EIP].byte[0] = decoded_instruction->total_length;

        #ifdef DEBUG
            printf("\nTotal instruction length: %hu\n", decoded_instruction->total_length);
        #endif

        if((*execution_handler_lut[decoded_instruction->opcode_id]) (decoded_instruction)) {
            printf("\nExecution complete\n");
        }
        
    }

    return 1;
}

int initialize(Program *program) {

    parse_file(program);
    initialize_ram(program);

    registers[EIP].dword = (uint32_t)ram_struct.text_base;
    registers[ESP].dword = (uint32_t)ram_struct.stack_base;

    return 1;
}


int main() {
    Program test;
    if (initialize(&test)) {
        print_contents(&test);
    }

    if(interpreter()) {
        printf("\nyay");
    }
    return 0;
}