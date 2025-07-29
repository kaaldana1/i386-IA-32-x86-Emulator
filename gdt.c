#include "gdt.h"
int create_gdt() {
    set_gdt_entry(&global_desc_table[NULL_DESCRIPTOR], 0, 0, 0x00, 0x00);
    set_gdt_entry(&global_desc_table[KERNEL_MODE_CODE_SEG], 0x0000, 0x07FF, KERNEL_CODE_RX, 0x04);
    set_gdt_entry(&global_desc_table[KERNEL_MODE_DATA_SEG], 0x0800, 0x07FF, KERNEL_DATA_RW, 0x04);
    set_gdt_entry(&global_desc_table[USER_MODE_CODE_SEG], 0x1000, 0x27FF, USER_CODE_RX, 0x04);
    set_gdt_entry(&global_desc_table[USER_MODE_DATA_STACK_SEG], 0x1800, 0x27FF, USER_DATA_RW, 0x04);
    return 1;
}