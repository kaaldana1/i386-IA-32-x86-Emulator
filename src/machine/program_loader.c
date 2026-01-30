#include "utils/utils.h"
#include "ui/display_api.h"
#include "machine/program_loader.h"



static FILE *get_file(char *filename)
{
    FILE *fptr = fopen(filename, "rb");
    if (fptr == NULL) 
    { 
        return NULL; 
    }
    return fptr;
}

Program *create_program(char *filename)
{
    Program *p = (Program*)calloc(1, sizeof(Program));
    FILE *fptr = get_file(filename);
    if (fptr == NULL) 
    {
        free(p);
        return NULL;
    }
    p->filename = fptr;
    return p;
}

static int parse_file(Program *program)
{
    FILE *fptr = program->filename;
    if (fptr == NULL) { return 0; }

    char buff[200];

    uint8Buffer tmp = {0};

    int i;
    uint8_t each;
    size_t bytes = 0;
    int offset = 0;

    while (fgets(buff, sizeof(buff), fptr))
    {
        i = 0;
        while (sscanf((buff + i), " %x%n", &each, &offset) == 1)
        {
            DA_APPEND(tmp, (uint8_t)each);
            bytes++;
            i += offset;
        }
    }
    fclose(fptr);

    program->arr = (uint8_t *)malloc(bytes);
    if (program->arr == NULL) { exit(1); }

    memcpy(program->arr, tmp.arr, bytes);
    program->size = bytes;
    
    if (ui_on)
        machine_state.ui_callbacks.ui_set_program_pointer(program);

    free(tmp.arr);
    return 1;
}

void print_contents(Program *program)
{
    if (program->arr == NULL)
    {
        //printw("Invalid");
        exit(1);
    }

    for (size_t i = 0; i < program->size; i++)
    {
        //printw("%02x ", program->arr[i]);
    }
    //printw("\nProgram size: %zu", program->arr_length);
}

static inline uint32_t create_dword(Program *program, size_t offset) 
{
    return ( ((uint32_t)program->arr[offset]) | ((uint32_t)program->arr[offset + 1]) << 8 | ((uint32_t)program->arr[offset + 2]) << 16 | ((uint32_t)program->arr[offset + 3]) << 24 );
}

int load_program(BUS *bus, Program *program, uint32_t code_addr) 
{
    if (parse_file(program) == 0) { return 0; }
    size_t offset = 0;
    while (offset + 4 < program->size) 
    {
        bus_write(bus, create_dword(program, offset), code_addr + offset, 32);
        offset += 4;
    }

    if (offset < program->size) 
    {
        uint32_t remainder_dword = 0; 
        uint32_t final_addr = code_addr + offset;
        int shift = 0;
        while (offset < program->size) 
        {
            remainder_dword |= ((uint32_t)program->arr[offset++]) << shift;
            shift += 8;
        }
        bus_write(bus, remainder_dword, final_addr, 32); 
    }

    return 1;
}
