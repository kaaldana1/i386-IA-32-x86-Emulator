#include "memory.h"

Ram_map ram_map;
uint8_t  ram_16kb[RAM_SIZE];
uint8_t  cpu_bus[4];

int initialize_ram(Program *p)
{
    memset(ram_16kb, 0x00, sizeof(ram_16kb));
    memset(&ram_map, 0, sizeof(Ram_map));
    // Load text portion of ram with the hex instructions
    ram_map.text_base = LOWEST_RAM_ADDRESS;
    memcpy(ram_16kb + ram_map.text_base, p->program, p->program_length);
    ram_map.text_size = p->program_length; // Max index would be prog_len-1

    ram_map.stack_base = HIGHEST_RAM_ADDRESS;
    // to also include global/static data
    ram_map.heap_base = (uint16_t)ram_map.text_size;
    return 1;
}

int memory_write_byte(uint8_t value, uint32_t address)
{
    // simulating writes to console port
    if (address == CONSOLE_PORT) {
        printf("%c", (unsigned char)value);
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
    memory_write_byte((0xFF & (value >> 8)), address);
    return 1;
}

int memory_write_dword(uint32_t value, uint32_t address) 
{ 
    memory_write_byte((0xFF & value), address);
    memory_write_byte((0xFF & (value >> 8)), address);
    memory_write_byte((0xFF & (value >> 16)), address);
    memory_write_byte((0xFF & (value >> 24)), address);

#ifdef DEBUG
    printf("Wrote word: \n");
    for (size_t i = (uint32_t)address; i < (uint32_t)address + 5; i++) {
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

