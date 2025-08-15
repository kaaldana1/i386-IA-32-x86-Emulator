#include "core/decoder.h"


#define OPERAND_SIZE_OVERRIDE 0x66
#define ADDR_SIZE_OVERRIDE 0x67
#define REP_REPE 0xF3
#define LOCK 0xF0
#define PREFIX_CS 0x2E

#define MAX_INSTR_LENGTH 15
#define MAX_PREFIX_LENGTH 4
#define MAX_OPCODE_LENGTH 3
#define MAX_MODRM_LENGTH 1
#define MAX_SIB_LENGTH 1
#define MAX_DISPLACEMENT_LENGTH 1
#define MAX_IMMEDIATE_LENGTH 4

#define MULT_BYTE_FLAG 0x0F

#define FIELD_PREFIX 0
#define FIELD_OPCODE 1
#define FIELD_MODRM 2
#define FIELD_SIB 3
#define FIELD_DISPLACEMENT 4
#define FIELD_IMMEDIATE 5



static void put_byte(Instruction *decoded_instr, int field, uint8_t byte)
{
    switch (field)
    {
    case FIELD_PREFIX:
        decoded_instr->prefix[decoded_instr->prefix_length++] = byte;
        break;
    case FIELD_OPCODE:
        decoded_instr->opcode[decoded_instr->opcode_length++] = byte;
        break;
    case FIELD_MODRM:
        decoded_instr->modrm = byte;
        decoded_instr->modrm_length++;
        break;
    case FIELD_SIB:
        decoded_instr->sib = byte;
        decoded_instr->sib_length++;
        break;
    case FIELD_DISPLACEMENT:
        decoded_instr->displacement[decoded_instr->displacement_length++] = byte;
        break;
    case FIELD_IMMEDIATE:
        decoded_instr->immediate[decoded_instr->immediate_length++] = byte;
        break;
    default:
        // bruh
        break;
    }
}

Instruction *decoder(uint8_t *encoded_instr)
{
    printf("\n\n =============Decoding==============\n");
    Instruction *decoded_instr = calloc(1, sizeof(Instruction));
    if (decoded_instr == NULL) { exit(1); }

    // Decode byte by byte
    uint8_t byte_p = 0, return_p = 0;
    while (byte_p < MAX_PREFIX_LENGTH)
    {
        bool add_byte = true;
        switch (encoded_instr[byte_p])
        {
        case OPERAND_SIZE_OVERRIDE:
            decoded_instr->has_operand_size_override = true;
            add_byte = true;
            break;
        case ADDR_SIZE_OVERRIDE:
            decoded_instr->has_addr_size_override = true;
            add_byte = true;
            break;
        case REP_REPE:
            decoded_instr->has_rep = true;
            add_byte = true;
            break;
        case LOCK:
            decoded_instr->has_lock = true;
            break;
        case PREFIX_CS: // deal with this later on
            break;
        default:
            add_byte = false;
            break;
        }

        if (add_byte)
        {
            put_byte(decoded_instr, FIELD_PREFIX, encoded_instr[byte_p]);
            byte_p++;
        }
        else
        {
            break;
        }
    }

    return_p = byte_p;

    while (byte_p < MAX_OPCODE_LENGTH + return_p)
    {
        // use lookup table
        if (!(encoded_instr[byte_p] == MULT_BYTE_FLAG))
        {
            Opcode_ID opcode = single_byte_opcode_lut[encoded_instr[byte_p]];

            put_byte(decoded_instr, FIELD_OPCODE, encoded_instr[byte_p]);
            byte_p++;
            decoded_instr->opcode_id = opcode;

#ifdef DEBUG
            printf("\nOPCODE is: %d\n", opcode);
            printf("Byte: %02x\n", decoded_instr->opcode[0]);
#endif

            break;
        }
        else
        {
            // multiple-byte instructions here
        }
    }

    return_p = byte_p;

    if (instr_metadata_lut[decoded_instr->opcode_id].has_modrm)
    {
        put_byte(decoded_instr, FIELD_MODRM, encoded_instr[byte_p]);
        byte_p++;

        uint8_t mod = (((decoded_instr->modrm) >> 6) & 0xFF);
        uint8_t reg_or_opcode = (((decoded_instr->modrm) >> 3) & 0x03);
        uint8_t rm_field = ((decoded_instr->modrm) & 0x03);

        decoded_instr->operands = operand_addr_form_lut[mod][rm_field][reg_or_opcode];

        decoded_instr->mod = mod;
        decoded_instr->reg_or_opcode = reg_or_opcode;
        decoded_instr->rm_field = rm_field;

#ifdef DEBUG
        printf("Has Imm: %zu\n", instr_metadata_lut[decoded_instr->opcode_id].immediate_bytes);
        printf("Modrm byte: %02x\n", decoded_instr->modrm);

        printf("Mod: %02x\n", decoded_instr->mod);
        printf("reg_or_opcode: %02x\n", decoded_instr->reg_or_opcode);
        printf("rm_field: %02x\n", decoded_instr->rm_field);

        printf("Addressing form: Reg 1: %d   .  Reg 2: %d", decoded_instr->operands.effective_addr_register, decoded_instr->operands.src_register);
#endif
    }

    return_p = byte_p;

    if (decoded_instr->sib_length != 0)
    {
    }

    return_p = byte_p;

    if (decoded_instr->displacement_length != 0)
    {
        while (byte_p < MAX_DISPLACEMENT_LENGTH)
        {
        }
    }

    return_p = byte_p;

    if (instr_metadata_lut[decoded_instr->opcode_id].immediate_bytes != 0) 
    {
        while (byte_p < instr_metadata_lut[decoded_instr->opcode_id].immediate_bytes + return_p) 
        {
            put_byte(decoded_instr, FIELD_IMMEDIATE, encoded_instr[byte_p]);
            byte_p++;
        }
    }

    return_p = byte_p;

    decoded_instr->total_length = decoded_instr->modrm_length +
                                  decoded_instr->opcode_length +
                                  decoded_instr->sib_length +
                                  decoded_instr->prefix_length +
                                  decoded_instr->displacement_length + 
                                  decoded_instr->immediate_length; 

    printf("\n\n ===========DECODING DONE============\n");
    return decoded_instr;
}

