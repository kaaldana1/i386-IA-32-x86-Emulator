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
#define MAX_DISPLACEMENT_LENGTH 4 
#define MAX_IMMEDIATE_LENGTH 4

#define MULT_BYTE_FLAG 0x0F

#define FIELD_PREFIX 0
#define FIELD_OPCODE 1
#define FIELD_MODRM 2
#define FIELD_SIB 3
#define FIELD_DISPLACEMENT 4
#define FIELD_IMMEDIATE 5

#define MOD_DISP8   0x01
#define MOD_DISP32  0x02
#define RM_SIB      0x04
#define RM_DISP32   0x05
#define SIB_BASE_NONE 0x05


typedef struct 
{
    uint8_t *encoded_instr;
    size_t byte_p;
} Cursor;

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

static void decode_prefix(Instruction *decoded_instr, Cursor *cursor)
{
    while (cursor->byte_p < MAX_PREFIX_LENGTH)
    {
        bool add_byte = true;
        switch (cursor->encoded_instr[cursor->byte_p])
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
            add_byte = true;
            break;
        case PREFIX_CS: // deal with this later on
            decoded_instr->has_CS = true;
            add_byte = true;
            break;
        default:
            add_byte = false;
            break;
        }

        if (add_byte)
        {
            put_byte(decoded_instr, FIELD_PREFIX, cursor->encoded_instr[cursor->byte_p++]);
        }
        else
        {
            break;
        }
    }

}

static void decode_opcode(Instruction *decoded_instr, Cursor *cursor)
{
    uint8_t return_p = cursor->byte_p;
    while (cursor->byte_p < (size_t)MAX_OPCODE_LENGTH + return_p)
    {
        // use lookup table
        if (!(cursor->encoded_instr[cursor->byte_p] == MULT_BYTE_FLAG))
        {
            OpcodeID opcode = single_byte_opcode_lut[cursor->encoded_instr[cursor->byte_p]];

            put_byte(decoded_instr, FIELD_OPCODE, cursor->encoded_instr[cursor->byte_p++]);
            decoded_instr->OpcodeID = opcode;

#ifdef DEBUG
            //printw("\nOPCODE is: %d\n", opcode);
            //printw("Byte: %02x\n", decoded_instr->opcode[0]);
#endif

            break;
        }
        else
        {
            // multiple-byte instructions here
        }
    }

}


static void decode_sib(Instruction *decoded_instr, Cursor *cursor)
{
        put_byte(decoded_instr, FIELD_SIB, cursor->encoded_instr[cursor->byte_p++]);

        decoded_instr->scale = ((decoded_instr->sib >> 6) & 0x3);
        decoded_instr->index =(((decoded_instr->sib) >> 3) & 0x7);
        decoded_instr->base = (((decoded_instr->sib)) & 0x7);

}


static void decode_modrm(Instruction *decoded_instr, Cursor *cursor)
{
    if (instr_metadata_lut[decoded_instr->OpcodeID].has_modrm)
    {
        put_byte(decoded_instr, FIELD_MODRM, cursor->encoded_instr[cursor->byte_p++]);

        uint8_t mod = (((decoded_instr->modrm) >> 6) & 0x3);
        uint8_t reg_or_opcode = (((decoded_instr->modrm) >> 3) & 0x7);
        uint8_t rm_field = ((decoded_instr->modrm) & 0x7);

        if ((mod != 3) && (rm_field == 0x4))
        {
            decode_sib(decoded_instr, cursor);
        }

        decoded_instr->mod = mod;
        decoded_instr->reg_or_opcode = reg_or_opcode;
        decoded_instr->rm_field = rm_field;

#ifdef DEBUG
        //printw("Has Imm: %zu\n", instr_metadata_lut[decoded_instr->OpcodeID].immediate_bytes);
        //printw("Modrm byte: %02x\n", decoded_instr->modrm);

        //printw("Mod: %02x\n", decoded_instr->mod);
        //printw("reg_or_opcode: %02x\n", decoded_instr->reg_or_opcode);
        //printw("rm_field: %02x\n", decoded_instr->rm_field);

#endif
    }


}

static void decode_disp(Instruction *decoded_instr, Cursor *cursor)
{
    if (!instr_metadata_lut[decoded_instr->OpcodeID].has_modrm) return;
    switch (decoded_instr->mod)
    {
        case (uint8_t)0x00:
            if ((decoded_instr->rm_field == RM_DISP32 && decoded_instr->sib_length == 0) || 
            ((decoded_instr->rm_field == RM_SIB) && (decoded_instr->base == SIB_BASE_NONE))) //32 bit displacement 
            {
                for (size_t i = 0; i < 4; i++) 
                {
                    put_byte(decoded_instr, FIELD_DISPLACEMENT, cursor->encoded_instr[cursor->byte_p++]);
                }
            }
        break;

        case MOD_DISP8: 
            put_byte(decoded_instr, FIELD_DISPLACEMENT, cursor->encoded_instr[cursor->byte_p++]);
        break; 

        case MOD_DISP32: 
            for (size_t i = 0; i < 4; i++) 
            {
                put_byte(decoded_instr, FIELD_DISPLACEMENT, cursor->encoded_instr[cursor->byte_p++]);
            }
        break; 
        
    }

}

static void decode_imm(Instruction *decoded_instr, Cursor *cursor)
{
    uint8_t return_p = cursor->byte_p;
    if (instr_metadata_lut[decoded_instr->OpcodeID].immediate_bytes != 0) 
    {
        while (cursor->byte_p < instr_metadata_lut[decoded_instr->OpcodeID].immediate_bytes + return_p) 
        {
            put_byte(decoded_instr, FIELD_IMMEDIATE, cursor->encoded_instr[cursor->byte_p++]);
        }
    }
}

Instruction *decoder(uint8_t *encoded_instr)
{
    //printw("/////////////////////////////////////////////////////////////////////\n");
    //printw("/////////////////////////////////////////////////////////////////////\n");
    //printw("/////////////////////////////////////////////////////////////////////\n");
    //printw("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    //printw("\n==================Decoding===================\n");


    Instruction *decoded_instr = calloc(1, sizeof(Instruction));
    if (decoded_instr == NULL) { return NULL; }

    Cursor cursor = (Cursor){encoded_instr, 0};

    // Decode byte by byte
    decode_prefix(decoded_instr, &cursor);
    decode_opcode(decoded_instr, &cursor);
    decode_modrm(decoded_instr, &cursor);
    decode_disp(decoded_instr, &cursor);
    decode_imm(decoded_instr, &cursor);


    decoded_instr->total_length = decoded_instr->modrm_length +
                                  decoded_instr->opcode_length +
                                  decoded_instr->sib_length +
                                  decoded_instr->prefix_length +
                                  decoded_instr->displacement_length + 
                                  decoded_instr->immediate_length; 

    //printw("\n==============Decoding DONE==================\n");
    return decoded_instr;
}

