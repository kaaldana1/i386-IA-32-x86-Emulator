#include "memory.h"

gdt_entry global_desc_table[FLAT_MODE_GDT_SIZE];
uint8_t  ram_16kb[RAM_SIZE];
uint8_t  cpu_bus[4];

int initialize_ram(Program *p)
{
    memset(ram_16kb, 0x00, sizeof(ram_16kb));
    set_gdt_entry(&global_desc_table[USER_MODE_CODE_SEG], (uint32_t)LOWEST_RAM_ADDRESS, (uint32_t)p->program_length, 0xFA, 0xC);
    set_gdt_entry(&global_desc_table[USER_MODE_DATA_SEG], (uint32_t)(LOWEST_RAM_ADDRESS + p->program_length), (uint32_t)(sizeof(ram_16kb) - p->program_length - 1), 0xF2, 0x0C);

    // Load text portion of ram with the hex instructions
    memcpy(ram_16kb + global_desc_table[USER_MODE_CODE_SEG].base_low, p->program, p->program_length);

    return 1;
}


int set_gdt_entry(gdt_entry *entry, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    memset(entry, 0, sizeof(gdt_entry));
    entry->base_low = base & 0xFFFF;
    entry->base_mid = (base >> 16) & 0xFF;
    entry->base_high = (base >> 24) & 0xFF;

    entry->limit_low = limit & 0xFFFF;
    entry->flag_limit_high =  ((flags & 0x0F) << 4) | ((limit >> 16) & 0x0F);

    entry->access_byte = access;
    return 0;
}


int memory_write_byte(uint8_t value, uint32_t address)
{
    // simulating writes to console port
    if (address == CONSOLE_PORT) {
        printf("=================================\n");
        printf("CONSOLE PRINT:\n");
        printf("%c", (unsigned char)value);
        printf("=================================\n");
        return 1;
    } else if (address <= HIGHEST_RAM_ADDRESS) {
        ram_16kb[address] = value;
        return 1;
    }
    return 0;
}

int memory_write_word(uint16_t value, uint32_t address) 
{
    memory_write_byte((0xFF & value), address);
    memory_write_byte((0xFF & (value >> 8)), address + 1);
    return 1;
}

int memory_write_dword(uint32_t value, uint32_t address) 
{     
    if (address == CONSOLE_PORT) {
        printf("=================================\n");
        printf("CONSOLE PRINT:\n");
        printf("%d\n", value);
        printf("=================================\n");
        return 1;
    } else {
        memory_write_byte((0xFF & value), address);
        memory_write_byte((0xFF & (value >> 8)), address + 1);
        memory_write_byte((0xFF & (value >> 16)), address + 2);
        memory_write_byte((0xFF & (value >> 24)), address + 3);
    }


#ifdef DEBUG
    printf("Wrote word: \n");
    for (size_t i = (size_t)address; i < (size_t)address + 5; i++) {
        printf("%02x  ", ram_16kb[i]);
    }
    printf("\n");
#endif

    return 1;
}

int memory_read_byte(uint8_t *value, uint32_t address) {
    char tmp; 
    if (address == KEYBOARD_PORT) {
        printf("Enter a char:\n");
        scanf("%c", &tmp);
        *value = (uint8_t)tmp;
        return 1;
    } else if (address <= HIGHEST_RAM_ADDRESS) {
        *value = ram_16kb[address];
        return 1;
    } else {
        return 0;
    }
}

int memory_read_dword (uint32_t *value, uint32_t address) {
    uint8_t byte1, byte2, byte3, byte4;
    if (!memory_read_byte(&byte1, address)) return 0;
    if (!memory_read_byte(&byte2, address + 1)) return 0;
    if (!memory_read_byte(&byte3, address + 2)) return 0;
    if (!memory_read_byte(&byte4, address + 3)) return 0;

    *value = (uint32_t)byte1 |
             (uint32_t)byte2 << 8 |
             (uint32_t)byte3 << 16 |
             (uint32_t)byte4 << 24;
    return 1;
}
