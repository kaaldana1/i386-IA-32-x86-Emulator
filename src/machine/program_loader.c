
#include "machine/program_loader.h"

struct Program {
    uint8_t *program;
    size_t program_length;
};

Program *create_program(void)
{
    Program *p = (Program*)calloc(1, sizeof(Program));
    return p;
}

static int parse_file(Program *p)
{
    FILE *fptr = fopen("src/machine/hex_code.txt", "rb");
    if (fptr == NULL) { printf("Error opening file!\n"); exit(1); }

    char buff[200];
    uint8_t *tmp = (uint8_t *)malloc(sizeof(buff));
    if (tmp == NULL) { exit(1); }

    int i, each;
    size_t bytes = 0;
    int offset = 0;

    while (fgets(buff, sizeof(buff), fptr))
    {
        i = 0;
        while (sscanf((buff + i), " %x%n", &each, &offset) == 1)
        {
            tmp[bytes++] = (uint8_t)each;
            i += offset;
        }
    }
    fclose(fptr);

    p->program = (uint8_t *)malloc(bytes);
    if (p->program == NULL) { exit(1); }

    memcpy(p->program, tmp, bytes);
    p->program_length = bytes;

    free(tmp);
    return 1;
}

void print_contents(Program *p)
{
    if (p->program == NULL)
    {
        printf("Invalid");
        exit(1);
    }

    for (size_t i = 0; i < p->program_length; i++)
    {
        printf("%02x ", p->program[i]);
    }
    printf("\nProgram size: %zu", p->program_length);
}

static inline uint32_t create_dword(Program *p, size_t offset) 
{
    return ( ((uint32_t)p->program[offset]) | ((uint32_t)p->program[offset + 1]) << 8 | ((uint32_t)p->program[offset + 2]) << 16 | ((uint32_t)p->program[offset + 3]) << 24 );
}

int load_program(BUS *bus, Program *p, uint32_t code_addr) 
{
    parse_file(p);
    size_t offset = 0;
    while (offset + 4 < p->program_length) 
    {
        bus_write(bus, create_dword(p, offset), code_addr + offset, 32);
        offset += 4;
    }

    if (offset < p->program_length) 
    {
        uint32_t remainder_dword = 0; 
        uint32_t final_addr = code_addr + offset;
        int remaining_bytes;
        int shift = 0;
        while (offset < p->program_length) 
        {
            remainder_dword |= ((uint32_t)p->program[offset++]) << shift;
            shift += 8;
        }
        bus_write(bus, remainder_dword, final_addr, 32); 
    }

    return 1;
}
