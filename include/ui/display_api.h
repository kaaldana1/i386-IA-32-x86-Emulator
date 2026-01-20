#include "core/structs/instruction.h" 
#include "machine/program_loader.h" 

extern bool ui_on;

typedef struct CPU CPU;
typedef struct RAMDev RAMDev;
typedef struct VGADev VGADev;
typedef struct KeyboardDev KeyboardDev;
////9 VGA;

typedef void (*copy_instr_after_decode)(const Instruction* instr);
typedef void (*copy_cpu_after_execute)(const CPU* cpu);
typedef void (*copy_mem_after_execute)(const uint8_t* mem, uint32_t address);
typedef void (*set_program_pointer)(Program *p);
typedef void (*set_display_ram_pointer)(const RAMDev *ram);
typedef void (*set_display_vga_pointer)(const VGADev *vga);
typedef void (*set_display_keyboard_pointer)(KeyboardDev *kb);
typedef void (*reset_stack_after_execute)();
typedef void (*copy_keyboard_input)(uint8_t key);
typedef void (*update_screen)();
typedef void (*flush_ui)();

typedef struct 
{
    copy_instr_after_decode   ui_copy_instr_after_decode;
    copy_cpu_after_execute    ui_copy_cpu_after_execute;
    copy_mem_after_execute    ui_copy_mem_after_execute;
    set_display_ram_pointer   ui_set_display_ram_ptr;
    set_display_vga_pointer   ui_set_display_vga_pointer;
    set_program_pointer       ui_set_program_pointer;
    reset_stack_after_execute ui_reset_stack_after_execute;
    set_display_keyboard_pointer ui_set_display_keyboard_pointer;
    copy_keyboard_input       ui_copy_keyboard_input;
    update_screen             ui_update_screen;
    flush_ui                  ui_flush_ui;
} UI_Callbacks;


typedef struct  
{
    CPU *cpu;
    uint8_t *memory;
    uint32_t mem_address;
    UI_Callbacks ui_callbacks;
} GlobalMachineState;

extern GlobalMachineState machine_state;