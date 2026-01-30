#include <unistd.h>
#include <wchar.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "ui/display.h"
#include "ui/display_api.h"
#include "ui/utils.h"
#include "utils/utils.h"
#include "core/cpu_internal.h"
#include "hardware_sim/devices_internal.h"
#include "ids/str_opcode_list.h"
#include "machine/program_loader.h"
#include "ids/register_ids.h"
#include "core/clock.h"


typedef struct 
{
    int height;
    int width;
    int y_offset;
    int x_offset;
} WindowLayout;

typedef enum
{
    WINDOW_REGISTERS = 0,
    WINDOW_DECODER,
    WINDOW_MEMORY,
    WINDOW_INSTRUCTION,
    WINDOW_PROGRAM,
    WINDOW_SCREEN,
    WINDOW_KEYBOARD,
    // WINDOW_TIME,
    WINDOW_COUNT
} WindowType;

const WindowLayout window_layouts[WINDOW_COUNT] = 
{
    [WINDOW_PROGRAM] = {15, 34, 0, 0},
    [WINDOW_DECODER] = {32, 34, 16, 0},
    [WINDOW_INSTRUCTION] = {4, 20, 48, 2},
    [WINDOW_REGISTERS] = {28, 67, 0, 35},
    [WINDOW_MEMORY] = {28, 75, 0, 103},
    [WINDOW_SCREEN] = {24, 67, 28, 35},
    [WINDOW_KEYBOARD] = {4, 10, 28, 102},
    // [WINDOW_TIME] = {4, 10, 32, 102}
};

typedef struct 
{
    size_t height;
    size_t width;
    int y_pos;
    int x_pos;
} ObjectDescription;

typedef enum
{
    DECODER_SIB_BOX,
    DECODER_MOD_BOX,
    DECODER_FIELDS,
    DECODER_FIELDS_ENTRY,
    SCREEN_COMPUTER_IMAGE,
    SCREEN_DRAWABLE_AREA,
    REGISTER_TABLE_AREA,
    REGISTER_TABLE_ENTRY,
    MEMORY_DATA_TABLE_AREA,
    MEMORY_DATA_TABLE_ENTRY,
    MEMORY_STACK_TABLE_AREA,
    MEMORY_STACK_TABLE_ENTRY,
    PROGRAM_DRAWABLE_AREA,

    OBJECT_COUNT
} ObjectType;

const ObjectDescription objects[OBJECT_COUNT] = 
{
    [DECODER_SIB_BOX]       = {.height = 4, .width = 17, .y_pos = 19, .x_pos = 2},
    [DECODER_MOD_BOX]       = {.height = 4, .width = 17, .y_pos = 24, .x_pos = 2},
    [DECODER_FIELDS]        = {.height = 1 /*col*/, .width = 4 /*row*/, .y_pos = 3, .x_pos = 2},
    [DECODER_FIELDS_ENTRY]  = {.height = 2, .width = 3, .y_pos = 0, .x_pos = 2},
    [SCREEN_COMPUTER_IMAGE] = {.height = 22, .width = 60, .y_pos = 1, .x_pos = 2},
    [SCREEN_DRAWABLE_AREA ] = {.height = SCREEN_HEIGHT, .width = SCREEN_WIDTH, .y_pos = 5, .x_pos = 30},
    [REGISTER_TABLE_AREA]   = {.height = 8 /*rows*/, .width = 3/*cols*/, .x_pos = 3, .y_pos = 2 },
    [REGISTER_TABLE_ENTRY]  = {.height = 3, .width = 20, .x_pos = 0, .y_pos = 0 },
    [MEMORY_DATA_TABLE_AREA]   = {.height = 12, .width = 4, .y_pos = 2, .x_pos = 10},
    [MEMORY_DATA_TABLE_ENTRY]  = {.height = 2, .width = 4, .y_pos = 0, .x_pos = 0 },
    [MEMORY_STACK_TABLE_AREA]  = {.height = 12, .width = 4, .y_pos = 2, .x_pos = 38 },
    [MEMORY_STACK_TABLE_ENTRY] = {.height = 2, .width = 4, .y_pos = 0 , .x_pos = 0 },
    [PROGRAM_DRAWABLE_AREA] = {.height = 13, .width = 30, .y_pos = 1, .x_pos = 2},

};

const int decoder_fields_count = 4;
const int mod_sib_fields_count = 6;

typedef enum 
{
    GPR_FIELD_X_POS = 0,
    SEGMENT_FIELD_X_POS,
    STATUS_FIELD_X_POS,
    HEADER_FIELDS_COUNT
} REGISTER_HEADER_FIELDS;

typedef enum
{
    CYAN_BLACK = 1,
    MAGENTA_BLACK,
    YELLOW_BLACK,
    GREEN_BLACK,
    RED_BLACK, 
    WHITE_BLACK,
    BLUE_BLACK,
    BLACK_WHITE,
    GREEN_WHITE
} MY_COLOR_PAIRS;

// static void print_word_binary(int win_num, int y_offset, int x_offset, char *print_buff);
static void print_byte_binary(int win_num, int y_offset, int x_offset, char *print_buff);
static void print_decoder_field(uint8_t *field, size_t field_size, int y_offset, int x_offset);
static void reset_decoder_window();
static void reset_memory_window();
// static void setup_clock_window();
static void setup_register_window();
static void setup_decoder_window();
static void setup_memory_window();
static void setup_instruction_window();
static void setup_screen_window();
static void setup_program_window();
static void setup_keyboard_window();
// static void update_time_window();
static void update_instruction_window();
static void update_register_window();
static void update_decoder_window();
static void update_memory_window();
static void update_stack();
static void draw_cell(int, int, uint16_t);
static void update_screen_window();
static void update_program_prev();
static void draw_partition_line(WINDOW *win, int x, int height);
static void create_table(WINDOW *win, int y0, int x0, int rows, int cols, int cell_w, int cell_h, const char *title);
void sleep_ms(long ms);
static void update_keyboard_window();

typedef struct 
{
    WINDOW *windows[WINDOW_COUNT];

    long time_delay_ms;
    CPU cpu;
    Instruction decoded_instr;

    long seconds_passed;

    uint8_t memory[4]; // legacy
    uint8_t *ram;
    uint32_t mem_address;

    Program *program;
    uint32_t last_byte_index_printed;
    uint32_t last_byte_index_decoded;
    uint32_t program_cursor;

    int instr_counter;

    VGADev *vga;
    uint8_t *preview_matrix;
    int pm_rows;
    int pm_cols;

    KeyboardDev *keyboard;
    uint8_t keyboard_input;

    bool altered_cpu;
    bool altered_memory;
    bool altered_instruction;
    bool altered_stack;
    bool altered_vram;
    bool altered_keyboard;
} UI_State;

static UI_State ui_state = {.windows = {NULL}, 
                            .cpu = {0}, .memory = {0}, 
                            .ram = NULL, .keyboard = NULL,
                            .seconds_passed = 0,
                            .decoded_instr = {0}, 
                            .program = NULL,
                            .last_byte_index_decoded = 0,
                            .last_byte_index_printed = 0,
                            .program_cursor = 0,
                            .instr_counter = 0,
                            .preview_matrix = NULL, 
                            .altered_cpu = false, 
                            .altered_memory = false, 
                            .altered_instruction = false,
                            .altered_stack= false};

//==========================================================================CALLBACKS=========================================================================
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//==========================================================================CALLBACKS=========================================================================
void cb_update_screen()
{
    ui_state.altered_vram = true;
}

void cb_copy_instr_after_decode(const Instruction* instr)
{
    ui_state.decoded_instr = *instr;
    ui_state.altered_instruction = true;
}

void cb_reset_stack_after_execute()
{
    ui_state.altered_stack = true;
}

void cb_copy_cpu_after_execute(const CPU* cpu)
{
    ui_state.cpu = *cpu;
    ui_state.altered_cpu = true;
}

void cb_copy_mem_after_execute(const uint8_t* mem, uint32_t address)
{
    memcpy(ui_state.memory, mem, 4);
    ui_state.mem_address = address;
    ui_state.altered_memory = true;
}

void cb_copy_keyboard_input(uint8_t key)
{
    ui_state.keyboard_input = key;
}

void cb_set_display_ram_pointer(const RAMDev *ram) 
{
    ui_state.ram = ram->ram_16kb;
}

void cb_set_program_pointer(Program *p)
{
    ui_state.program = p;
    update_program_prev();
}

void cb_set_display_vga_pointer(const struct VGADev *vga)
{
    ui_state.vga = vga;
}

void cb_set_display_keyboard_pointer(KeyboardDev *keyboard)
{
    ui_state.keyboard = keyboard;
}

void cb_flush_ui()
{
    update_keyboard_window();
    // flush per new instruction
    // update_time_window();
    update_instruction_window();

    // for each instruction, update the last_byte_decoded = EIP 
    //   window:
    //   int first_byte_printed = ((sizeof(Matrix)-1) <= last_byte_printed) ? (last_byte_printed - sizeof(Matrix) - 1) : 0;
    //   bool EIP_before_prev = (EIP < first_byte_printed);
    //   bool EIP_after_prev =  (EIP > last_byte_printed);
    //  if (EIP_before_prev || EIP_after_prev)
    //      program_cursor = EIP, last_byte_index_printed = (EIP == 0) ? 0 : EIP - 1 // retreat program stream
            // update_program_prev:
    //              iterate bytes_printed and program_cursor until matrix is filled or program cursor reaches the end 
    //              ui_state.last_byte_index_printed += bytes_in_matrix; 

    uint32_t eip = ui_state.cpu.gen_purpose_registers[EIP].dword;
    size_t matrix_size = (size_t)ui_state.pm_rows * (size_t)ui_state.pm_cols;
    uint32_t first_byte_printed = (ui_state.last_byte_index_printed >= (uint32_t)(matrix_size - 1)) ? (ui_state.last_byte_index_printed - (matrix_size - 1) ) : 0;
    bool EIP_before_prev = (eip < first_byte_printed);
    bool EIP_after_prev = (eip > ui_state.last_byte_index_printed);
    if (EIP_before_prev || EIP_after_prev)
    {
        ui_state.program_cursor = eip;
        ui_state.last_byte_index_printed = (eip == 0 )? 0 : eip - 1; // this is the previous window 
        update_program_prev();
    }

    if (ui_state.altered_vram)
    {
        update_screen_window();
        ui_state.altered_vram = false;
    }

    if (ui_state.altered_cpu) 
    {
        update_register_window();
        ui_state.altered_cpu = false;
    }

    if (ui_state.altered_memory) 
    {
        update_memory_window();
        ui_state.altered_memory = false;
    }
    else
    {
        reset_memory_window();
        ui_state.altered_memory = false;
    }

    if (ui_state.altered_instruction) 
    {
        reset_decoder_window();
        update_decoder_window();
        ui_state.altered_instruction = false;
    }

    if (ui_state.altered_stack)
    {
        update_stack();
        ui_state.altered_stack = false;
    }

    // not liking how an actual part of the emulator depends on ui. but for now its ok
    sleep_ms(ui_state.time_delay_ms);

}

int init_ui(long time_delay_ms) 
{
    initscr();
    cbreak(); // exits out on ctrl+c
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    refresh();

    ui_state.time_delay_ms = time_delay_ms;

    start_color();
    init_pair(CYAN_BLACK, COLOR_CYAN, COLOR_BLACK);
    init_pair(MAGENTA_BLACK, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(YELLOW_BLACK, COLOR_YELLOW, COLOR_BLACK);
    init_pair(GREEN_BLACK, COLOR_GREEN, COLOR_BLACK);
    init_pair(RED_BLACK, COLOR_RED, COLOR_BLACK);
    init_pair(WHITE_BLACK, COLOR_WHITE, COLOR_BLACK);
    init_pair(BLUE_BLACK, COLOR_BLUE, COLOR_BLACK);
    init_pair(BLACK_WHITE, COLOR_BLACK, COLOR_WHITE);
    init_pair(GREEN_WHITE, COLOR_GREEN, COLOR_WHITE);

    for (int i = 0; i < WINDOW_COUNT; i++)
    {
        const WindowLayout *layout = &window_layouts[i];
        ui_state.windows[i] = newwin(layout->height, layout->width, layout->y_offset, layout->x_offset);
        if (ui_state.windows[i] == NULL)
        {
            destroy_window();
            return 0;
        }
    }

    keypad(ui_state.windows[0], TRUE);

    for (int i = 0; i < WINDOW_COUNT; i++) 
    {
        box(ui_state.windows[i], 0, 0);
        wrefresh(ui_state.windows[i]);
    }

    // setup_clock_window();
    setup_register_window();
    setup_decoder_window();
    setup_memory_window();
    setup_instruction_window();
    setup_screen_window();
    setup_program_window();
    setup_keyboard_window();

    UI_Callbacks *cb = &(machine_state.ui_callbacks);

    *cb = (UI_Callbacks){
        .ui_copy_instr_after_decode = cb_copy_instr_after_decode,
        .ui_copy_cpu_after_execute = cb_copy_cpu_after_execute,
        .ui_copy_mem_after_execute = cb_copy_mem_after_execute,
        .ui_set_display_ram_ptr = cb_set_display_ram_pointer,
        .ui_set_display_vga_pointer = cb_set_display_vga_pointer,
        .ui_set_display_keyboard_pointer = cb_set_display_keyboard_pointer,
        .ui_set_program_pointer = cb_set_program_pointer,
        .ui_flush_ui = cb_flush_ui,
        .ui_reset_stack_after_execute = cb_reset_stack_after_execute,
        .ui_copy_keyboard_input = cb_copy_keyboard_input,
        .ui_update_screen = cb_update_screen,
    };

    if (cb->ui_copy_instr_after_decode == NULL || cb->ui_copy_cpu_after_execute == NULL ||
        cb->ui_copy_mem_after_execute == NULL || cb->ui_set_display_ram_ptr == NULL ||
        cb->ui_set_display_vga_pointer == NULL || cb->ui_set_display_keyboard_pointer == NULL ||
        cb->ui_set_program_pointer == NULL || cb->ui_flush_ui == NULL ||
        cb->ui_reset_stack_after_execute == NULL || cb->ui_copy_keyboard_input == NULL ||
        cb->ui_update_screen == NULL)
    {
        return 0;
    }

    return 1;

}

void destroy_window() 
{
    for (int i = 0; i < WINDOW_COUNT; i++)
        delwin(ui_state.windows[i]);
    endwin();
}

//==========================================================================INSTRUCTION=========================================================================
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//==========================================================================INSTRUCTION=========================================================================

static void setup_instruction_window()
{ 
    werase(ui_state.windows[WINDOW_INSTRUCTION]);
    box(ui_state.windows[WINDOW_INSTRUCTION], 0, 0);
    wattron(ui_state.windows[WINDOW_INSTRUCTION], A_STANDOUT);
    mvwprintw(ui_state.windows[WINDOW_INSTRUCTION], 0, 1, "INSTRUCTION");
    wattroff(ui_state.windows[WINDOW_INSTRUCTION], A_STANDOUT);
    wrefresh(ui_state.windows[WINDOW_INSTRUCTION]);
}

static void update_instruction_window()
{
    // TEMPORARY: only for single byte opcode 
    mvwprintw(ui_state.windows[WINDOW_INSTRUCTION], 1, 2, "%-17s", string_opcode_list[ui_state.decoded_instr.opcode[0]]);
    wrefresh(ui_state.windows[WINDOW_INSTRUCTION]);
}


//==========================================================================REGISTER=========================================================================
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//==========================================================================REGISTER=========================================================================

static void create_registers_table()
{
    int divider_amount              = objects[REGISTER_TABLE_AREA].height - 2;
    int bottom_header_line_y_offset = objects[REGISTER_TABLE_AREA].y_pos + objects[REGISTER_TABLE_ENTRY].height;

    WINDOW *win = ui_state.windows[WINDOW_REGISTERS];
    wattron(win, COLOR_PAIR(1));
    create_table(win, objects[REGISTER_TABLE_AREA].y_pos, objects[REGISTER_TABLE_AREA].x_pos, objects[REGISTER_TABLE_AREA].height, objects[REGISTER_TABLE_AREA].width, objects[REGISTER_TABLE_ENTRY].width, objects[REGISTER_TABLE_ENTRY].height, "REGISTER TABLE");

    int header_text_y_offset        = (objects[REGISTER_TABLE_AREA].y_pos + 2);
    int gen_header_x_offset         = (objects[REGISTER_TABLE_AREA].x_pos + 2)  + (GPR_FIELD_X_POS)      * objects[REGISTER_TABLE_ENTRY].width;
    int seg_header_x_offset         = (objects[REGISTER_TABLE_AREA].x_pos + 2)  + (SEGMENT_FIELD_X_POS)  * objects[REGISTER_TABLE_ENTRY].width;
    int sr_header_x_offset          = (objects[REGISTER_TABLE_AREA].x_pos + 2)  + (STATUS_FIELD_X_POS)   * objects[REGISTER_TABLE_ENTRY].width;

    // header, bottom horizontal at y = 5
    for (size_t i = 0; i < objects[REGISTER_TABLE_AREA].width; i++)
    {
        if (i == 0) mvwprintw(ui_state.windows[WINDOW_REGISTERS], header_text_y_offset, gen_header_x_offset, "GPR");
        else if (i == 1) mvwprintw(ui_state.windows[WINDOW_REGISTERS], header_text_y_offset, seg_header_x_offset, "SR");
        else if (i == 2) mvwprintw(ui_state.windows[WINDOW_REGISTERS], header_text_y_offset, sr_header_x_offset, "Status");
    }

    wattroff(ui_state.windows[WINDOW_REGISTERS], COLOR_PAIR(1));

    const int gpr_amount = 9;
    const int seg_amount = 6;

    const char *gpr_names[] = { 
        "EAX","ECX","EDX","EBX","EIP","EBP","ESI","EDI","ESP"
    };
    const char *sr_names[] = {"CS","DS","ES","FS","GS","SS"};
    const char *flag_names[] = {"CF","PF","AF","ZF","SF","OF"};

    wattron(ui_state.windows[WINDOW_REGISTERS], COLOR_PAIR(2));
    for (int i = 0; i < gpr_amount - 1;  i++) 
    {

        if (strcmp(gpr_names[i], "EIP") == 0) 
        {
            mvwprintw(ui_state.windows[WINDOW_REGISTERS], bottom_header_line_y_offset + 1 + objects[REGISTER_TABLE_ENTRY].height*divider_amount, seg_header_x_offset, " %-8s ", gpr_names[i]);
            mvwprintw(ui_state.windows[WINDOW_REGISTERS], bottom_header_line_y_offset + 2 + objects[REGISTER_TABLE_ENTRY].height*divider_amount, seg_header_x_offset, " CODE_PTR ");
        }
        else if (strcmp(gpr_names[i], "ESP") == 0)
        {
            mvwprintw(ui_state.windows[WINDOW_REGISTERS], bottom_header_line_y_offset + 1 + objects[REGISTER_TABLE_ENTRY].height*divider_amount, sr_header_x_offset, " %-8s ", gpr_names[i]);
            mvwprintw(ui_state.windows[WINDOW_REGISTERS], bottom_header_line_y_offset + 2 + objects[REGISTER_TABLE_ENTRY].height*divider_amount, sr_header_x_offset, " STACK_PTR ");
        }
        else if (strcmp(gpr_names[i], "EDI") == 0)
        {
            mvwprintw(ui_state.windows[WINDOW_REGISTERS], bottom_header_line_y_offset + 1 + objects[REGISTER_TABLE_ENTRY].height*4, gen_header_x_offset, " %-8s ", gpr_names[i]);
        }
        else
        {
            mvwprintw(ui_state.windows[WINDOW_REGISTERS], bottom_header_line_y_offset + 1 + objects[REGISTER_TABLE_ENTRY].height*i, gen_header_x_offset, " %-8s ", gpr_names[i]);
        }

        if (i < seg_amount) 
        {
            mvwprintw(ui_state.windows[WINDOW_REGISTERS], bottom_header_line_y_offset + 1 + objects[REGISTER_TABLE_ENTRY].height*i, seg_header_x_offset, " %-8s  ", sr_names[i]);
            mvwprintw(ui_state.windows[WINDOW_REGISTERS], bottom_header_line_y_offset + 1 + objects[REGISTER_TABLE_ENTRY].height*i, sr_header_x_offset, " %-8s  ", flag_names[i]);
        }
    }
    wattroff(ui_state.windows[WINDOW_REGISTERS], COLOR_PAIR(2));
}

static void setup_register_window() 
{
    werase(ui_state.windows[WINDOW_REGISTERS]);
    box(ui_state.windows[WINDOW_REGISTERS], 0, 0);

    wattron(ui_state.windows[WINDOW_REGISTERS], A_STANDOUT);
    mvwprintw(ui_state.windows[WINDOW_REGISTERS], 0, 1, "REGISTERS");
    wattroff(ui_state.windows[WINDOW_REGISTERS], A_STANDOUT);

    create_registers_table(ui_state.windows[WINDOW_REGISTERS]);
    wrefresh(ui_state.windows[WINDOW_REGISTERS]);
    update_register_window();
}

static void update_register_window()
{
    int divider_amount              = objects[REGISTER_TABLE_AREA].height - 2;
    int bottom_header_line_y_offset = objects[REGISTER_TABLE_AREA].y_pos + objects[REGISTER_TABLE_ENTRY].height;
    int gen_header_x_offset         = (objects[REGISTER_TABLE_AREA].x_pos + 2)  + (GPR_FIELD_X_POS)      * objects[REGISTER_TABLE_ENTRY].width;
    int seg_header_x_offset         = (objects[REGISTER_TABLE_AREA].x_pos + 2)  + (SEGMENT_FIELD_X_POS)  * objects[REGISTER_TABLE_ENTRY].width;
    int sr_header_x_offset          = (objects[REGISTER_TABLE_AREA].x_pos + 2)  + (STATUS_FIELD_X_POS)   * objects[REGISTER_TABLE_ENTRY].width;

    for (int i = 0; i < 9; i++) 
    {
        char buf[9];
        snprintf(buf, sizeof(buf), "%08x", ui_state.cpu.gen_purpose_registers[i].dword);
        if ( i == 4 ) 
        {
            mvwprintw(ui_state.windows[WINDOW_REGISTERS], bottom_header_line_y_offset + 1 + objects[REGISTER_TABLE_ENTRY].height*divider_amount, seg_header_x_offset + 5, " %-8.8s ", buf);
            mvwprintw(ui_state.windows[WINDOW_REGISTERS], bottom_header_line_y_offset + 2 + objects[REGISTER_TABLE_ENTRY].height*divider_amount, seg_header_x_offset, " CODE_PTR ");
        }
        else if ( i == 8 )
        {
            mvwprintw(ui_state.windows[WINDOW_REGISTERS], bottom_header_line_y_offset + 1 + objects[REGISTER_TABLE_ENTRY].height*divider_amount, sr_header_x_offset + 5, " %-8.8s ", buf);
            mvwprintw(ui_state.windows[WINDOW_REGISTERS], bottom_header_line_y_offset + 2 + objects[REGISTER_TABLE_ENTRY].height*divider_amount, sr_header_x_offset, " STACK_PTR ");
        }
        else if ( i == 7 )
        {
            mvwprintw(ui_state.windows[WINDOW_REGISTERS], bottom_header_line_y_offset + 1 + objects[REGISTER_TABLE_ENTRY].height*4, gen_header_x_offset + 5, "%-8.8s", buf);
        }
        else
        {
            mvwprintw(ui_state.windows[WINDOW_REGISTERS], bottom_header_line_y_offset + 1 + objects[REGISTER_TABLE_ENTRY].height*i, gen_header_x_offset + 5, "%-8.8s", buf);
        }
        if (i < 6) 
        {
            mvwprintw(ui_state.windows[WINDOW_REGISTERS], bottom_header_line_y_offset + 1 + objects[REGISTER_TABLE_ENTRY].height*i, seg_header_x_offset + 4, " %x  ", ui_state.cpu.segment_registers[i].selector);
            mvwprintw(ui_state.windows[WINDOW_REGISTERS], bottom_header_line_y_offset + 1 + objects[REGISTER_TABLE_ENTRY].height*i, sr_header_x_offset + 4, " %x  ", (ui_state.cpu.status_register >> i) & 1);
        }
    }
    wrefresh(ui_state.windows[WINDOW_REGISTERS]);
}

//==========================================================================DECODER=========================================================================
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//==========================================================================DECODER=========================================================================


static void setup_decoder_window()
{
    WINDOW *win = ui_state.windows[WINDOW_DECODER];
    werase(ui_state.windows[WINDOW_DECODER]);
    box(ui_state.windows[WINDOW_DECODER], 0, 0);

    wattron(ui_state.windows[WINDOW_DECODER], A_STANDOUT);
    mvwprintw(ui_state.windows[WINDOW_DECODER], 0, 1, "DECODER");
    wattroff(ui_state.windows[WINDOW_DECODER], A_STANDOUT);

    mvwprintw(ui_state.windows[WINDOW_DECODER], 1, 1, "Instruction:");

    char *decoder_field_names[] = {"PREFIX", "OPC and ModRM", "Immediate", "Displacement"};

    for (int i = 0; i < decoder_fields_count; i++)
        create_table(win, objects[DECODER_FIELDS].y_pos + 4*i, objects[DECODER_FIELDS].x_pos, objects[DECODER_FIELDS].height, objects[DECODER_FIELDS].width, objects[DECODER_FIELDS_ENTRY].width, objects[DECODER_FIELDS_ENTRY].height, decoder_field_names[i]);
    
    create_table(win, objects[DECODER_MOD_BOX].y_pos, objects[DECODER_MOD_BOX].x_pos, 1, 1, objects[DECODER_MOD_BOX].width, objects[DECODER_MOD_BOX].height, "MOD");
    create_table(win, objects[DECODER_SIB_BOX].y_pos, objects[DECODER_SIB_BOX].x_pos, 1, 1, objects[DECODER_SIB_BOX].width, objects[DECODER_SIB_BOX].height, "SIB");


    char *mod_and_sib_field_names[] = {"Scale:", "Index:", "Base:", "Mod:", "Reg:", "RM:"};
    for (size_t i = 0; i < (sizeof(mod_and_sib_field_names) / sizeof(mod_and_sib_field_names[0])); i++)
    {
        if (i < 3) mvwprintw(ui_state.windows[WINDOW_DECODER], objects[DECODER_SIB_BOX].y_pos + i + 1, objects[DECODER_SIB_BOX].x_pos + 1, "%s", mod_and_sib_field_names[i]);
        else  mvwprintw(ui_state.windows[WINDOW_DECODER], objects[DECODER_MOD_BOX].y_pos + i - 2, objects[DECODER_SIB_BOX].x_pos + 1, "%s", mod_and_sib_field_names[i]);
    }

    mvwprintw(ui_state.windows[WINDOW_DECODER], objects[DECODER_MOD_BOX].y_pos + 6, objects[DECODER_MOD_BOX].x_pos + 1, "Instruction Length:");

    wrefresh(ui_state.windows[WINDOW_DECODER]);
    update_decoder_window();
}

static void print_decoder_field(uint8_t *field, size_t field_size, int y_offset, int x_offset)
{
    for (size_t i = 0; i < field_size; i++)
        mvwprintw(ui_state.windows[WINDOW_DECODER], y_offset, x_offset + 3*i, "%02x", field[i]);
}

static void reset_decoder_window()
{
    uint8_t zero_buff_4[4] = {0};
    for (int i = 0; i < decoder_fields_count; i++)
        print_decoder_field(zero_buff_4, objects[DECODER_FIELDS].width, objects[DECODER_FIELDS].y_pos + 1 + 4*i, objects[DECODER_FIELDS].x_pos + 1);

    char zero_buff_32[32] = {0};

    for (int i = 0; i < mod_sib_fields_count; i++)
    {
        if (i < 3) print_byte_binary(WINDOW_DECODER, objects[DECODER_SIB_BOX].y_pos + 1 + i, objects[DECODER_SIB_BOX].x_pos + 8, zero_buff_32);
        else  print_byte_binary(WINDOW_DECODER, objects[DECODER_MOD_BOX].y_pos + i - 2, objects[DECODER_MOD_BOX].x_pos + 8, zero_buff_32);
    }

    mvwprintw(ui_state.windows[WINDOW_DECODER], objects[DECODER_MOD_BOX].y_pos + 6, objects[DECODER_MOD_BOX].x_pos + 20, "%x", 0);
    wrefresh(ui_state.windows[WINDOW_DECODER]);
}

static void update_decoder_window() 
{
    mvwprintw(ui_state.windows[WINDOW_DECODER], 1, 16, "%i", ui_state.instr_counter++);

    uint8_t *fields[] = { &ui_state.decoded_instr.prefix, &ui_state.decoded_instr.opcode, &ui_state.decoded_instr.modrm, &ui_state.decoded_instr.immediate, &ui_state.decoded_instr.displacement };
    uint8_t op_index = 1; uint8_t mod_index = 2;
/*hallo*/
    for (size_t i = 0; i < (sizeof(fields)/sizeof(fields[0])); i++)
    {
        //dumb code but its pretty fun
        uint8_t op_or_mod = ((op_index | mod_index) & i);
        size_t size = (op_or_mod == 0) ? 4 : ((op_or_mod == op_index) ? 3 : 1);
        int jump_x_offs = (op_or_mod == mod_index) ? 9 : 0;
        int same_y_offs = (op_or_mod == mod_index) ? i : 1;
        bool retreat_y_offs = (i > mod_index) ? 1 : 0;
        print_decoder_field(fields[i], size, objects[DECODER_FIELDS].y_pos + 1 + 4*(i-retreat_y_offs)/same_y_offs, objects[DECODER_FIELDS].x_pos + 1 + jump_x_offs);
    }

    char print_buff[32];
    uint8_t mod_and_sib_fields[] = {ui_state.decoded_instr.scale, ui_state.decoded_instr.index, 
                                    ui_state.decoded_instr.base, ui_state.decoded_instr.mod, 
                                    ui_state.decoded_instr.reg_or_opcode, ui_state.decoded_instr.rm_field};

    for (size_t i = 0; i < sizeof(mod_and_sib_fields); i++)
    {
        full_hex_to_binary(mod_and_sib_fields[i], print_buff);
        if (i < 3) print_byte_binary(WINDOW_DECODER, objects[DECODER_SIB_BOX].y_pos + 1 + i, objects[DECODER_SIB_BOX].x_pos + 8, print_buff);
        else  print_byte_binary(WINDOW_DECODER, objects[DECODER_MOD_BOX].y_pos + i - 2, objects[DECODER_MOD_BOX].x_pos + 8, print_buff);
    }

    mvwprintw(ui_state.windows[WINDOW_DECODER], objects[DECODER_MOD_BOX].y_pos + 6, objects[DECODER_MOD_BOX].x_pos + 20, "%x", ui_state.decoded_instr.total_length);
    wrefresh(ui_state.windows[WINDOW_DECODER]);
}

//==========================================================================MEMORY=========================================================================
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//==========================================================================MEMORY=========================================================================
static void setup_memory_window()
{
    WINDOW *win = ui_state.windows[WINDOW_MEMORY];
    const int win_h = window_layouts[WINDOW_MEMORY].height;

    werase(win);
    box(win, 0, 0);

    wattron(win, A_STANDOUT);
    mvwprintw(win, 0, 0, "MEMORY");
    wattroff(win, A_STANDOUT);

    // Address column
    const int address_column_width = 8;
    mvwprintw(win, 1, 1, "ADDRESS");
    draw_partition_line(win, address_column_width, win_h);


    create_table(win, objects[MEMORY_DATA_TABLE_AREA].y_pos, objects[MEMORY_DATA_TABLE_AREA].x_pos,  objects[MEMORY_DATA_TABLE_AREA].height, objects[MEMORY_DATA_TABLE_AREA].width, objects[MEMORY_DATA_TABLE_ENTRY].width, objects[MEMORY_DATA_TABLE_ENTRY].height, "DATA");

    // Stack partition 

    mvwprintw(win, 1, objects[MEMORY_STACK_TABLE_AREA].x_pos - 9, "ADDRESS");
    draw_partition_line(win, objects[MEMORY_STACK_TABLE_AREA].x_pos - 10, win_h);
    draw_partition_line(win, objects[MEMORY_STACK_TABLE_AREA].x_pos - 2, win_h);
    create_table(win, objects[MEMORY_STACK_TABLE_AREA].y_pos, objects[MEMORY_STACK_TABLE_AREA].x_pos,  objects[MEMORY_STACK_TABLE_AREA].height, objects[MEMORY_STACK_TABLE_AREA].width, objects[MEMORY_STACK_TABLE_ENTRY].width, objects[MEMORY_STACK_TABLE_ENTRY].height, "STACK");
    update_stack();


    wrefresh(win);
    update_memory_window();
}

static void update_stack()
{
    const int stack_y_offset = objects[MEMORY_STACK_TABLE_AREA].y_pos;
    const int stack_x_offset = objects[MEMORY_STACK_TABLE_AREA].x_pos;;
    for (int row = 0; row < 12; row++)
    {
        mvwprintw(ui_state.windows[WINDOW_MEMORY], stack_y_offset + 2 * row + 1, 30 + 1, "%x", (((0x1800 + 0x27FF)) - (row * 4)));
        for (int col = 0; col < 4; col++)
        {
            if (ui_state.ram != NULL)
                mvwprintw(ui_state.windows[WINDOW_MEMORY], stack_y_offset + 2 * row + 1, stack_x_offset + 1 + 4*col, "%02x", ui_state.ram[(((0x1800 + 0x27FF) + col) - (row * 4))]);
            else
                mvwprintw(ui_state.windows[WINDOW_MEMORY], stack_y_offset + 2 * row + 1, stack_x_offset + 1 + 4*col, "00");
        }
    }
}


static void update_memory_window()
{
    mvwprintw(ui_state.windows[WINDOW_MEMORY], 3, 2, "%x", ui_state.mem_address);

    if (ui_state.altered_memory)
    {
        const int target_row = 6;
        for (int row = 0; row < 12; row++)
        {
            if (row == 6) 
                wattron(ui_state.windows[WINDOW_MEMORY], COLOR_PAIR(5));
            int address = (ui_state.mem_address) - ((target_row - row) * 4);
            if (address >= 0)
                mvwprintw(ui_state.windows[WINDOW_MEMORY], (objects[MEMORY_DATA_TABLE_AREA].y_pos + 2 * row) + 1, 2, "%04x", address);
            else
                mvwprintw(ui_state.windows[WINDOW_MEMORY], (objects[MEMORY_DATA_TABLE_AREA].y_pos + 2 * row) + 1, 2, "%04x", 0);
            for (int i = 0; i < 4; i++) 
            {
                int extended_data_address = address + i;
                mvwprintw(ui_state.windows[WINDOW_MEMORY], (objects[MEMORY_DATA_TABLE_AREA].y_pos + 2 * row) + 1, objects[MEMORY_DATA_TABLE_AREA].x_pos + 1 + 4*i, "%02x", ui_state.ram[(extended_data_address >= 0) ? extended_data_address : 0]);
            }
            if (row == 6) 
                wattroff(ui_state.windows[WINDOW_MEMORY], COLOR_PAIR(5));
        }
    }

    wrefresh(ui_state.windows[WINDOW_MEMORY]);
}

static void reset_memory_window()
{

    for (int row = 0; row < 12; row++)
    {
        if (row == 6) wattron(ui_state.windows[WINDOW_MEMORY], COLOR_PAIR(5));
        mvwprintw(ui_state.windows[WINDOW_MEMORY], (objects[MEMORY_DATA_TABLE_AREA].y_pos + 2 * row) + 1, 2, "0000");
        for (int i = 0; i < 4; i++) 
        {
            mvwprintw(ui_state.windows[WINDOW_MEMORY], (objects[MEMORY_DATA_TABLE_AREA].y_pos + 2 * row) + 1, objects[MEMORY_DATA_TABLE_AREA].x_pos + 1 + 4*i, "00");
        }
        if (row == 6) wattroff(ui_state.windows[WINDOW_MEMORY], COLOR_PAIR(5));
    }

    wrefresh(ui_state.windows[WINDOW_MEMORY]);
}

//======================================================================COMPUTER SCREEN====================================================================
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================COMPUTER SCREEN====================================================================

static void setup_screen_window()
{ 
    werase(ui_state.windows[WINDOW_SCREEN]);
    box(ui_state.windows[WINDOW_SCREEN], 0, 0);
    wattron(ui_state.windows[WINDOW_SCREEN], A_STANDOUT);
    mvwprintw(ui_state.windows[WINDOW_SCREEN], 0, 1, "CONSOLE");
    wattroff(ui_state.windows[WINDOW_SCREEN], A_STANDOUT);

    const int pixel_art_start_y = 1;
    const int pixel_art_start_x = 2;
    const int pixel_art_height = 22;
    // const int pixel_art_width =  60;

    const char *screen_art[] =   
    {
            "████████████████████████████████████████████████████▀▀▀▀▀▀▀█",
            "███████████████████████████████▀▀▀▀▀▀▀▀▀▀▀▄▄▄▄▄▄▄▄▄▄█████│╙▐",
            "██████████▀▀╙╙╙╙│▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄█████████████████████████│╙▐",
            "█████████▌╙╙╙╙╙╙│████████████████████████████████████████│╙▐",
            "███████▀╙▌╙╙╙╙╙╙│████████████████████████████████████████│╙▐",
            "████▀╙╙╙╙▌╙╙╙╙╙╙│████████████████████████████████████████│╙▐",
            "██▌╙╙╙╙╙╙▌╙╙╙╙╙╙│████████████████████████████████████████│╙▐",
            "██╙╙╙════▌╙╙╙╙╙╙│████████████████████████████████████████│╙▐",
            "██╙╙╙════▌╙╙╙╙╙╙│████████████████████████████████████████│╙▐",
            "█▌╙╙╙════▌╙╙╙╙╙╙│████████████████████████████████████████│╙▐",
            "█▌╙╙╙════▌╙╙╙╙╙╙│████████████████████████████████████████│╙▐",
            "█▌╙╙╙════▌╙╙╙╙╙╙│████████████████████████████████████████│╙▐",
            "█▌╙╙╙╙═══▌╙╙╙╙╙╙│████████████████████████████████████████│╙▐",
            "█▌╙╙╙╙╙╙╙▌╙╙╙╙╙╙│████████████████████████████████████████│╙▐",
            "██▄▄▄╙╙╙╙▌╙╙╙╙╙╙│████████████████████████████████████████│╙▐",
            "████╙█╙╙╙▌╙╙╙╙╙╙│████████████████████████████████████████│╙▐",
            "████══▀▄╙▌╙╙╙╙╙╙│████████████████████████████████████████│╙▐",
            "█████════▌╙╙╙╙╙╙│████████████████████████████████████▀▀▀▀│╙▐",
            "███████▄▄▌╙╙╙╙╙╙│█████████████████▀▀▀▀▀▀▀▀▀┼─────────┼───┼╙▐",
            "██████████╙╙╙╙╙╙┼────────╬─╬╬╬╬───┼────────┼▄▄▄▄▄▄▄▄▄███████",
            "███████████▄▄╙╙╙┼───────┼▄▄▄▄▄▄▄▄▄▄█████████████████████████",
            "████████████████████████████████████████████████████████████"
    };


    wattron(ui_state.windows[WINDOW_SCREEN], COLOR_PAIR(8));

    for (int i = 0; i < pixel_art_height; i++) 
    {
        mvwprintw(ui_state.windows[WINDOW_SCREEN], pixel_art_start_y + i, pixel_art_start_x, "%s", screen_art[i]);
    }

    wattroff(ui_state.windows[WINDOW_SCREEN], COLOR_PAIR(8));

    /*
    wattron(ui_state.windows[WINDOW_SCREEN], COLOR_PAIR(4));
    mvwprintw(ui_state.windows[WINDOW_SCREEN], pixel_art_height/2, pixel_art_start_x + 3*pixel_art_width/5, "%s", ">HELLO");
    mvwprintw(ui_state.windows[WINDOW_SCREEN], pixel_art_height/2 + 1, pixel_art_start_x + 3*pixel_art_width/5, "%s", ">_");
    wattron(ui_state.windows[WINDOW_SCREEN], COLOR_PAIR(4));
    */
    wrefresh(ui_state.windows[WINDOW_SCREEN]);
}



static void draw_cell(int x, int y, uint16_t value)
{
    WINDOW *win_screen = ui_state.windows[WINDOW_SCREEN];
    uint8_t c = (uint8_t)value;
    uint8_t attribute = (uint8_t)(value >> 8);
    if (attribute >= 1 && attribute <= 9) wattron(win_screen, COLOR_PAIR(attribute));
    mvwprintw(win_screen, objects[SCREEN_DRAWABLE_AREA].y_pos + y, objects[SCREEN_DRAWABLE_AREA].x_pos + x, "%c", (char)c);
    if (attribute >= 9) wattroff(win_screen, COLOR_PAIR(attribute));
}


static void update_screen_window()
{
    VGADev *v = ui_state.vga;
    uint32_t cell_index;
    for (int i=0; i < v->counter_vram_changes; i++)
    {
        cell_index = v->dirty[i];
        coord c = find_cell(cell_index);
        draw_cell(c.x, c.y, v->vram[c.y][c.x]);
        v->dirty_map[cell_index] = false;
    }
    v->counter_vram_changes = 0;
    wrefresh(ui_state.windows[WINDOW_SCREEN]);
}

//======================================================================PROGRAM====================================================================
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================PROGRAM====================================================================


static inline uint8_t *get_matrix_index(int row, int col)
{
    return &ui_state.preview_matrix[(size_t)row * ui_state.pm_cols + col];
}


static void draw_matrix()
{
    for (int row = 0; row < ui_state.pm_rows; row++)
    {
        for (int col = 0; col < ui_state.pm_cols; col++)
        {
            mvwprintw(ui_state.windows[WINDOW_PROGRAM], objects[PROGRAM_DRAWABLE_AREA].y_pos + row, objects[PROGRAM_DRAWABLE_AREA].x_pos + col * 3, "%02x ", *get_matrix_index(row, col));
        }
    }
}

static void repopulate_matrix()
{
    memset(ui_state.preview_matrix, 0, (size_t)ui_state.pm_rows * ui_state.pm_cols);
    
    for (int row = 0; row < ui_state.pm_rows; row++)
    {
        for (int col = 0; col < ui_state.pm_cols; col++)
        {
            if ((size_t)ui_state.program_cursor >= ui_state.program->size)  goto done;
            *get_matrix_index(row, col) = ui_state.program->arr[ui_state.program_cursor++];
        }
    }
    done:
    ui_state.last_byte_index_printed = (ui_state.program_cursor == 0) ? 0 : (ui_state.program_cursor - 1);
}

static void update_program_prev()
{
    repopulate_matrix();
    draw_matrix();
    wrefresh(ui_state.windows[WINDOW_PROGRAM]);
}

static void setup_program_window() 
{
    werase(ui_state.windows[WINDOW_PROGRAM]);
    box(ui_state.windows[WINDOW_PROGRAM], 0, 0);

    wattron(ui_state.windows[WINDOW_PROGRAM], A_STANDOUT);
    mvwprintw(ui_state.windows[WINDOW_PROGRAM], 0, 1, "PROGRAM");
    wattroff(ui_state.windows[WINDOW_PROGRAM], A_STANDOUT);

    ui_state.pm_rows = objects[PROGRAM_DRAWABLE_AREA].height;
    ui_state.pm_cols = objects[PROGRAM_DRAWABLE_AREA].width / 3;

    ui_state.preview_matrix = calloc((size_t)ui_state.pm_rows * ui_state.pm_cols, 1);
    draw_matrix();
    wrefresh(ui_state.windows[WINDOW_PROGRAM]);
}


//======================================================================KEYBOARD====================================================================
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================KEYBOARD====================================================================

static void setup_keyboard_window()
{
    werase(ui_state.windows[WINDOW_KEYBOARD]);
    box(ui_state.windows[WINDOW_KEYBOARD], 0, 0);

    wattron(ui_state.windows[WINDOW_KEYBOARD], A_STANDOUT);
    mvwprintw(ui_state.windows[WINDOW_KEYBOARD], 0, 1, "KEYBOARD");
    wattroff(ui_state.windows[WINDOW_KEYBOARD], A_STANDOUT);

    wrefresh(ui_state.windows[WINDOW_KEYBOARD]);
}

static void update_keyboard_window()
{
    mvwprintw(ui_state.windows[WINDOW_KEYBOARD], 2, 2, "%c", (char)ui_state.keyboard_input);
    wrefresh(ui_state.windows[WINDOW_KEYBOARD]);
}

//======================================================================CLOCK====================================================================
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================CLOCK====================================================================
/*
static void setup_clock_window()
{
    werase(ui_state.windows[WINDOW_TIME]);
    box(ui_state.windows[WINDOW_TIME], 0, 0);

    wattron(ui_state.windows[WINDOW_TIME], A_STANDOUT);
    mvwprintw(ui_state.windows[WINDOW_TIME], 0, 1, "TIME");
    wattroff(ui_state.windows[WINDOW_TIME], A_STANDOUT);

    mvwprintw(ui_state.windows[WINDOW_TIME], 2, 2, "%04lu", ui_state.seconds_passed);

    wrefresh(ui_state.windows[WINDOW_TIME]);

}

static void update_time_window()
{
    if ((clock % 2100) == 0)
    {
        ui_state.seconds_passed++;
        mvwprintw(ui_state.windows[WINDOW_TIME], 2, 2, "%04lu", ui_state.seconds_passed);
    }
    wrefresh(ui_state.windows[WINDOW_TIME]);
}
*/

// ============================================================HELPERS==============================================================
static void create_table(WINDOW *win, int y0, int x0, int rows, int cols, int cell_w, int cell_h, const char *title)
{
    const int width  = cols * cell_w;
    const int height = rows * cell_h;

    if (title) mvwprintw(win, y0 - 1, x0, "%s", title);

    // Top border
    mvwhline(win, y0, x0, ACS_HLINE, width);
    mvwaddch(win, y0, x0, ACS_ULCORNER);
    mvwaddch(win, y0, x0 + width, ACS_URCORNER);

    // top col separators 
    for (int c = 1; c < cols; c++) 
    {
        mvwaddch(win, y0, x0 + c * cell_w, ACS_TTEE);
    }

    // between rows
    for (int r = 1; r < rows; r++) 
    {
        int y = y0 + r * cell_h;
        mvwhline(win, y, x0, ACS_HLINE, width);
        mvwaddch(win, y, x0, ACS_LTEE);
        mvwaddch(win, y, x0 + width, ACS_RTEE);

        for (int c = 1; c < cols; c++) {
            mvwaddch(win, y, x0 + c * cell_w, ACS_PLUS);
        }
    }

    // Bottom border
    const int yb = y0 + height;
    mvwhline(win, yb, x0, ACS_HLINE, width);
    mvwaddch(win, yb, x0, ACS_LLCORNER);
    mvwaddch(win, yb, x0 + width, ACS_LRCORNER);

    // bottom column separators
    for (int c = 1; c < cols; c++) 
        mvwaddch(win, yb, x0 + c * cell_w, ACS_BTEE);

    // Vertical lines 
    mvwvline(win, y0 + 1, x0, ACS_VLINE, height - 1);
    mvwvline(win, y0 + 1, x0 + width, ACS_VLINE, height - 1);

    for (int c = 1; c < cols; c++) 
        mvwvline(win, y0 + 1, x0 + c * cell_w, ACS_VLINE, height - 1);
}

static void draw_partition_line(WINDOW *win, int x, int height)
{
    mvwhline(win, 0, x, ACS_TTEE, 1);
    for (int i = 0; i < height - 1; i++) {
        mvwhline(win, 1 + i, x, ACS_VLINE, 1);
    }
    mvwhline(win, height - 1, x, ACS_BTEE, 1);
}

// static void print_word_binary(int win_num, int y_offset, int x_offset, char *print_buff)
// {
//     for (int i = 0; i < 16; i++)
//     {
//         mvwprintw(ui_state.windows[win_num], y_offset, x_offset + i, "%i", (int)print_buff[16+i]);
//     }
// }

static void print_byte_binary(int win_num, int y_offset, int x_offset, char *print_buff)
{
    for (int i = 0; i < 8; i++)
    {
        mvwprintw(ui_state.windows[win_num], y_offset, x_offset + i, "%i", (int)print_buff[24+i]);
    }
}

void sleep_ms(long ms)
{
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}
