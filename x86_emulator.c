#include "x86_emulator.h"

int interpreter(Program *p) {
    int start_addr;
    while (registers[EIP].dword < p->program_length) {
        start_addr = registers[EIP].dword;

        uint8_t instr_buff[MAX_INSTR_LENGTH];
        memset(instr_buff, 0, sizeof(uint8_t));

        // Assume MAX instruction length, and truncate accordingly
        memcpy(instr_buff, (p->program + start_addr), 15);
        Instruction *decoded_instruction = decoder(instr_buff);
        registers[EIP].byte[0] = decoded_instruction->total_length;

        #ifdef DEBUG
            printf("\nTotal instruction length: %hu", decoded_instruction->total_length);
        #endif

        if((*execution_handler_lut[decoded_instruction->opcode_id]) (decoded_instruction)) {
            printf("\nExecution complete\n");
        }
        
    }

    return 0;
}



int main() {
    Program test;
    if (parse_file(&test)) {
        print_contents(&test);
    }

    if(interpreter(&test)) {
        printf("yay");
    }
    return 0;
}