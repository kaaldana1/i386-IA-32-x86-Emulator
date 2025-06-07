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

Instruction * decoder(uint8_t *single_instr) {
    Instruction *decoded_instr = malloc(sizeof(Instruction));
    if (decoded_instr == NULL) { printf("nah"); exit(1); }
    memset(decoded_instr, 0, sizeof(Instruction));

    // Decode byte by byte 
    uint8_t byte_p = 0, return_p = 0;
    while(byte_p < MAX_PREFIX_LENGTH) {
        bool add_byte = true;
        switch(single_instr[byte_p]) {
            case OPERAND_SIZE_OVERRIDE: decoded_instr->has_operand_size_override = true; add_byte = true;
                break;
            case ADDR_SIZE_OVERRIDE: decoded_instr->has_addr_size_override = true; add_byte = true;
                break;
            case REP_REPE: decoded_instr->has_rep = true; add_byte = true;
                break;
            case LOCK: decoded_instr->has_lock = true;
                break;
            case CS: // deal with this later on
                break;
            default:
                add_byte = false;
                break;
        }

        if (add_byte) {
            put_byte(decoded_instr, FIELD_PREFIX, single_instr[byte_p]);
            byte_p++;
        } else { break; }
    }

    return_p = byte_p;

    while (byte_p < MAX_OPCODE_LENGTH + return_p) {
        // use lookup table
        if (!(single_instr[byte_p] == MULT_BYTE_FLAG)) {
            Opcode_ID opcode = single_byte_opcode_lut[single_instr[byte_p]];

            put_byte(decoded_instr, FIELD_OPCODE, single_instr[byte_p]);
            byte_p++;
            decoded_instr->opcode_id = opcode;

            #ifdef DEBUG
                printf("\nOPCODE is: %d\n", opcode);
                printf("Byte: %02x\n", decoded_instr->opcode[0]);
            #endif

            break;

        } else { 
            // multiple-byte instructions here
        }
    }

    return_p = byte_p;

    if (instr_metadata_lut[decoded_instr->opcode_id].has_modrm) {
        put_byte(decoded_instr, FIELD_MODRM, single_instr[byte_p]);
        byte_p++;

        uint8_t mod = (((decoded_instr->modrm) >> 6) & 0xFF);
        uint8_t reg_or_opcode = (((decoded_instr->modrm) >> 3) & 0x03);
        uint8_t rm_field = ((decoded_instr->modrm) & 0x03);

        decoded_instr->operands = operand_addr_form_lut[mod][rm_field][reg_or_opcode];

        decoded_instr->mod  = mod;
        decoded_instr->reg_or_opcode = reg_or_opcode;
        decoded_instr->rm_field = rm_field;

        #ifdef DEBUG
            printf("Has Imm: %d\n", instr_metadata_lut[decoded_instr->opcode_id].has_immediate);
            printf("Modrm byte: %02x\n", decoded_instr->modrm);

            printf("Mod: %02x\n", decoded_instr->mod);
            printf("reg_or_opcode: %02x\n", decoded_instr->reg_or_opcode);
            printf("rm_field: %02x\n", decoded_instr->rm_field);

            printf("Addressing form: Reg 1: %d   .  Reg 2: %d", decoded_instr->operands.reg_1, decoded_instr->operands.reg_2);
        #endif
    }

    if (instr_metadata_lut[decoded_instr->opcode_id].has_sib) { }

    if (instr_metadata_lut[decoded_instr->opcode_id].has_displacement) {
        while (byte_p < MAX_DISPLACEMENT_LENGTH) { }
    }

    if (instr_metadata_lut[decoded_instr->opcode_id].has_immediate) { 
        while(byte_p < MAX_IMMEDIATE_LENGTH) { }
    }

    decoded_instr->total_length= decoded_instr->displacement_length + 
                                decoded_instr->modrm_length +
                                decoded_instr->opcode_length + 
                                decoded_instr->sib_length + 
                                decoded_instr->prefix_length +
                                decoded_instr->displacement_length;

   return decoded_instr;
}

void put_byte(Instruction *decoded_instr, int field, uint8_t byte) {
    switch(field) {
        case FIELD_PREFIX: decoded_instr->prefix[decoded_instr->prefix_length++] = byte;
            break;
        case FIELD_OPCODE: decoded_instr->opcode[decoded_instr->opcode_length++] = byte;
            break;
        case FIELD_MODRM: decoded_instr->modrm = byte; decoded_instr->modrm_length++;
            break;
            case FIELD_SIB: decoded_instr->sib = byte; decoded_instr->sib_length++;
            break;
        case FIELD_DISPLACEMENT: decoded_instr->displacement[decoded_instr->displacement_length++]= byte;
            break;
        case FIELD_IMMEDIATE: decoded_instr->immediate[decoded_instr->immediate_length++] = byte;
            break;
        default:
            // bruh
            break;
    }
}


int execute_ADD_RM8_R8(Instruction *decoded_instr){return 0;}
int execute_ADD_RM32_R32 (Instruction *decoded_instr){ printf("\nExecuting ADD RM32 R32..."); return 1; }
int execute_ADD_R8_RM8 (Instruction *decoded_instr){return 0;}
int execute_ADD_R32_RM32 (Instruction *decoded_instr){return 0;}
int execute_ADD_AL_IMM8 (Instruction *decoded_instr){return 0;}
int execute_ADD_EAX_IMM32(Instruction *decoded_instr){return 0;}



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