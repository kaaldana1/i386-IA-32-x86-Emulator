#include "machine/display.h"
#include "machine/display_api.h"
#include "machine/utils.h"
#include "core/cpu_internal.h"


static void setup_register_window();
static void setup_decoder_window();
static void setup_memory_window();
static void update_register_window();
static void update_decoder_window();
static void update_memory_window();

typedef struct 
{
    WINDOW *windows[3];

    CPU cpu;
    Instruction decoded_instr;
    uint8_t memory[4];
    uint32_t mem_address;

    bool altered_cpu;
    bool altered_memory;
    bool altered_instruction;
} UI_State;

static UI_State ui_state = {.windows = {NULL, NULL, NULL}, 
                            .cpu = {0}, .memory = {0}, 
                            .decoded_instr = {0}, 
                            .altered_cpu = false, 
                            .altered_memory = false, 
                            .altered_instruction = false};

// =========================================================================
// Update functions called by the core after certain events
// =========================================================================
void cb_copy_instr_after_decode(const Instruction* instr)
{
    ui_state.decoded_instr = *instr;
    ui_state.altered_instruction = true;
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

void cb_flush_ui()
{
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
    if (ui_state.altered_instruction) 
    {
        update_decoder_window();
        ui_state.altered_instruction = false;
    }
}

// =========================================================================
// =========================================================================

static void print_word_binary(int win_num, int y_offset, int x_offset, char *print_buff)
{
    for (int i = 0; i < 16; i++)
    {
        mvwprintw(ui_state.windows[win_num], y_offset, x_offset + i, "%i", (int)print_buff[16+i]);
    }
}

static void print_byte_binary(int win_num, int y_offset, int x_offset, char *print_buff)
{
    for (int i = 0; i < 8; i++)
    {
        mvwprintw(ui_state.windows[win_num], y_offset, x_offset + i, "%i", (int)print_buff[24+i]);
    }
}

void init_ui() 
{
    initscr();
    cbreak(); // exits out on ctrl+c
    noecho();
    start_color();

    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_MAGENTA, COLOR_BLACK);

    ui_state.windows[1] = newwin(15, 60, 0, 0);
    ui_state.windows[2] = newwin(15, 40, 0, 61);
    ui_state.windows[0] = newwin(36, 101, 15, 0);

    keypad(ui_state.windows[0], TRUE);

    for (int i = 0; i < 3; i++) 
    {
        box(ui_state.windows[i], 0, 0);
        wrefresh(ui_state.windows[i]);
    }

    wattron(ui_state.windows[0], A_STANDOUT);
    mvwprintw(ui_state.windows[0], 0, 1, " i386 Emulator ");
    wattroff(ui_state.windows[0], A_STANDOUT);

    wattron(ui_state.windows[0], COLOR_PAIR(1));
    mvwprintw(ui_state.windows[0], 1, 1, "Press 'c' to continue...");
    wattroff(ui_state.windows[0], COLOR_PAIR(1));
    wrefresh(ui_state.windows[0]);

    setup_register_window();
    setup_decoder_window();
    setup_memory_window();
    
    machine_state.ui_callbacks.ui_copy_instr_after_decode = cb_copy_instr_after_decode;
    machine_state.ui_callbacks.ui_copy_cpu_after_execute = cb_copy_cpu_after_execute;
    machine_state.ui_callbacks.ui_copy_mem_after_execute = cb_copy_mem_after_execute;

}

static void create_registers_table()
{
    wattron(ui_state.windows[0], COLOR_PAIR(1));
    mvwhline(ui_state.windows[0], 2, 3, ACS_ULCORNER, 1);
    for (int i = 0; i < 3 ; i++)
    {
        mvwhline(ui_state.windows[0], 2, 4 + 30*i, ACS_HLINE, 30);
        if (i == 1 || i == 2) mvwhline(ui_state.windows[0], 2, 3 + 30*i, ACS_TTEE, 1);
    }
    mvwhline(ui_state.windows[0], 2, 3 + 30*3, ACS_URCORNER, 1);


    // header, bottom horizontal at y = 5
    for (int i = 0; i < 3 ; i++)
    {
        if (i == 0) mvwprintw(ui_state.windows[0], 4, 6 + 30*i, "General Purpose Registers");
        else if (i == 1) mvwprintw(ui_state.windows[0], 4, 10 + 30*i, "Segment Registers");
        else if (i == 2) mvwprintw(ui_state.windows[0], 4, 11 + 30*i, "Status Registers");
        mvwhline(ui_state.windows[0], 5, 3 + 30*i, ACS_HLINE, 30);
        mvwhline(ui_state.windows[0], 5, 3 + 30*i, ACS_PLUS, 1);
    }

    // dividers for each reg
    for (int j = 0; j < 8; j++)
    {
        for (int i = 0; i < 3 ; i++)
        {
            mvwhline(ui_state.windows[0], 8 + 3*j, 3 + 30*i, ACS_HLINE, 30);
            mvwhline(ui_state.windows[0], 8 + 3*j, 3 + 30*i, ACS_PLUS, 1);
        }
    }

    mvwhline(ui_state.windows[0], 33, 3, ACS_LLCORNER, 1);
    for (int i = 0; i < 3 ; i++)
    {
        mvwhline(ui_state.windows[0], 33, 4 + 30*i, ACS_HLINE, 30);
        if (i == 1 || i == 2) mvwhline(ui_state.windows[0], 33, 3 + 30*i, ACS_BTEE, 1);
    }
    mvwhline(ui_state.windows[0], 33, 3 + 30*3, ACS_LRCORNER, 1);

    // vertical lines
    for(int j = 0; j < 4; j++)
    {
        for(int i = 0; i < 30; i++)
            mvwhline(ui_state.windows[0], 3 + i, 3 + j*30, ACS_VLINE, 1);
    }
    wattroff(ui_state.windows[0], COLOR_PAIR(1));

    const char *gpr_names[] = { 
        "EAX","ECX","EDX","EBX","EIP","EBP","ESI","EDI","ESP"
    };
    const char *sr_names[] = {"CS","DS","ES","FS","GS","SS"};
    const char *flag_names[] = {"CF","PF","AF","ZF","SF","OF"};

    wattron(ui_state.windows[0], COLOR_PAIR(2));
    for (int i = 0; i < 9; i++) 
    {
        mvwprintw(ui_state.windows[0], 6 + 3*i, 4, " %-8s ", gpr_names[i]);
        if (i < 6) 
        {
            mvwprintw(ui_state.windows[0], 6 + 3*i, 34, " %-8s  ", sr_names[i]);
            mvwprintw(ui_state.windows[0], 6 + 3*i, 64, " %-8s  ", flag_names[i]);
        }
    }
    wattroff(ui_state.windows[0], COLOR_PAIR(2));
}

static void setup_register_window() 
{
    werase(ui_state.windows[0]);
    box(ui_state.windows[0], 0, 0);

    wattron(ui_state.windows[0], A_STANDOUT);
    mvwprintw(ui_state.windows[0], 0, 1, "REGISTERS");
    wattroff(ui_state.windows[0], A_STANDOUT);

    create_registers_table(ui_state.windows[0]);
    wrefresh(ui_state.windows[0]);
    update_register_window();
}

static void update_register_window()
{
    for (int i = 0; i < 9; i++) 
    {

        mvwprintw(ui_state.windows[0], 6 + 3*i, 14, " %x ", ui_state.cpu.gen_purpose_registers[i].dword);
        if (i < 6) 
        {
            mvwprintw(ui_state.windows[0], 6 + 3*i, 44, " %x  ", ui_state.cpu.segment_registers[i].selector);
            mvwprintw(ui_state.windows[0], 6 + 3*i, 74, " %x  ", ui_state.cpu.status_register & (1 << i));
        }
    }
    wrefresh(ui_state.windows[0]);
}


static void setup_decoder_window()
{
    werase(ui_state.windows[1]);
    box(ui_state.windows[1], 0, 0);

    wattron(ui_state.windows[1], A_STANDOUT);
    mvwprintw(ui_state.windows[1], 0, 1, "DECODER");
    wattroff(ui_state.windows[1], A_STANDOUT);

    mvwprintw(ui_state.windows[1], 3, 2, "Prefix ");
    mvwprintw(ui_state.windows[1], 6, 2, "Opc & ModRM");
    mvwprintw(ui_state.windows[1], 9, 2, "Immediate ");
    mvwprintw(ui_state.windows[1], 12, 2, "Displacement ");

    for (int j = 0; j < 4; j++) 
    {
        for (int i = 0; i <= 4; i++) 
            mvwhline(ui_state.windows[1], 3 + (3 *j), 15 + 4*i, ACS_VLINE, 1);
    }

    
    for (int i = 0; i < 4; i++)
    {
        mvwhline(ui_state.windows[1], 2 + (3*i), 16, ACS_HLINE, 16);
        mvwhline(ui_state.windows[1], 4 + (3*i), 16,  ACS_HLINE, 16);

        mvwhline(ui_state.windows[1], 4 + (3*i), 15, ACS_LLCORNER, 1);
        mvwhline(ui_state.windows[1], 4 + (3*i), 31, ACS_LRCORNER, 1);
        mvwhline(ui_state.windows[1], 2 + (3*i), 15, ACS_ULCORNER, 1);
        mvwhline(ui_state.windows[1], 2 + (3*i), 31, ACS_URCORNER, 1);
        
        for (int j = 1; j < 4; j++) 
        {
            mvwhline(ui_state.windows[1], 2 + (3*i), 15 + 4 * j, ACS_TTEE, 1);
            mvwhline(ui_state.windows[1], 4 + (3*i), 15 + 4 * j, ACS_BTEE, 1);
        }
    }

    for (int k = 0; k < 2; k++) 
    {
        for (int j = 0; j < 2; j++) 
        {
            for (int i = 0; i < 3; i++) 
                mvwhline(ui_state.windows[1], 3 + (5*k) + i, 34 + 18*j, ACS_VLINE, 1);
        }
    }

    mvwhline(ui_state.windows[1], 2, 35, ACS_HLINE, 17);
    mvwhline(ui_state.windows[1], 6, 35,  ACS_HLINE, 17);
    mvwhline(ui_state.windows[1], 6, 34, ACS_LLCORNER, 1);
    mvwhline(ui_state.windows[1], 6, 52, ACS_LRCORNER, 1);
    mvwhline(ui_state.windows[1], 2, 34, ACS_ULCORNER, 1);
    mvwhline(ui_state.windows[1], 2, 52, ACS_URCORNER, 1);
    
    mvwhline(ui_state.windows[1], 7, 35, ACS_HLINE, 17);
    mvwhline(ui_state.windows[1], 11, 35,  ACS_HLINE, 17);
    mvwhline(ui_state.windows[1], 11, 34, ACS_LLCORNER, 1);
    mvwhline(ui_state.windows[1], 11, 52, ACS_LRCORNER, 1);
    mvwhline(ui_state.windows[1], 7, 34, ACS_ULCORNER, 1);
    mvwhline(ui_state.windows[1], 7, 52, ACS_URCORNER, 1);

    wattron(ui_state.windows[1], A_STANDOUT);
    mvwprintw(ui_state.windows[1], 2, 35, "SIB");
    wattroff(ui_state.windows[1], A_STANDOUT);
    
    uint32_t test_scale = 0xb312;
    char print_buff[32];
    full_hex_to_binary(test_scale, print_buff);
    print_byte_binary(1, 3, 43, print_buff);

    mvwprintw(ui_state.windows[1], 3, 36, "Scale:");
    mvwprintw(ui_state.windows[1], 4, 36, "Index:");
    mvwprintw(ui_state.windows[1], 5, 36, "Base:");

    wattron(ui_state.windows[1], A_STANDOUT);
    mvwprintw(ui_state.windows[1], 7, 35, "MOD");
    wattroff(ui_state.windows[1], A_STANDOUT);
    mvwprintw(ui_state.windows[1], 8, 36, "Mod:");
    mvwprintw(ui_state.windows[1], 9, 36, "Reg:");
    mvwprintw(ui_state.windows[1], 10, 36, "RM:");

    
    wrefresh(ui_state.windows[1]);
    update_decoder_window();
}

static void update_decoder_window() 
{
    uint8_t mem[4] = {1, 0xb, 3, 4};
    for (int i = 0; i < 4; i++)
        mvwprintw(ui_state.windows[1], 3, 17 + 4 * i, "%x", mem[i]);

    wrefresh(ui_state.windows[1]);
}

static void setup_memory_window()
{
    werase(ui_state.windows[2]);
    box(ui_state.windows[2], 0, 0);

    wattron(ui_state.windows[2], A_STANDOUT);
    mvwprintw(ui_state.windows[2], 0, 1, "MEMORY");
    wattroff(ui_state.windows[2], A_STANDOUT);

    wrefresh(ui_state.windows[2]);
}



static void update_memory_window(WINDOW *win)
{
    return 1;
}

void destroy_window() {
    delwin(ui_state.windows[0]);
    delwin(ui_state.windows[1]);
    delwin(ui_state.windows[2]);
    endwin();
}
