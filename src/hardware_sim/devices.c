#include "hardware_sim/devices_internal.h"
#include "ui/display_api.h"

RAMDev *init_ram(void)
{
    RAMDev *ram_dev = (RAMDev*)calloc(1, sizeof(RAMDev));
    memset(ram_dev->ram_16kb, 0x00, RAM_SIZE);
    machine_state.ui_callbacks.ui_set_display_ram_ptr(ram_dev);
    return ram_dev;
}

CONSOLEDev *init_console(void)
{
    CONSOLEDev *c = (CONSOLEDev*)calloc(1, sizeof(CONSOLEDev));
    memset(c->data, 0, CONSOLE_SIZE);
    return c;
}

VGADev *init_vga(void)
{
    VGADev *v = (VGADev*)calloc(1, sizeof(VGADev));
    memset(v, 0, sizeof(VGADev));
    machine_state.ui_callbacks.ui_set_display_vga_pointer(v);
    return v;
}

int ram_write_byte(RAMDev *r, uint8_t value, uint32_t address) 
{
    r->ram_16kb[address] = value;
    return 1;
}

int ram_write_word(RAMDev *r, uint16_t value, uint32_t address) 
{
    ram_write_byte(r, (uint8_t)value, address);
    ram_write_byte(r, (uint8_t)(value >> 8), address + 1);
    return 1;
}

int ram_write_dword(void *ram_dev, uint32_t value, uint32_t address) 
{
    RAMDev *r = ram_dev;
    if (address > RAM_SIZE - 4) { return 0; }
    ram_write_word(r, (uint16_t)(value & 0xFFFF), address);
    ram_write_word(r, (uint16_t)(value >> 16), address + 2);
    return 1;
} 

int ram_read_byte (RAMDev *r, uint8_t *value, uint32_t address)
{
    *value = r->ram_16kb[address];
    return 1;
}

int ram_read_word(RAMDev *r, uint16_t *value, uint32_t address) 
{
    uint8_t low, high;
    ram_read_byte(r, &low, address);
    ram_read_byte(r, &high, address + 1);
    *value = (uint16_t)low | ((uint16_t)high << 8);
    return 1;
}


int ram_read_dword(void *ram_dev, uint32_t *value, uint32_t address) 
{
    RAMDev *r = ram_dev;
    if (address > RAM_SIZE - 4) { return 0; }
    uint16_t low, high;
    ram_read_word(r, &low, address);
    ram_read_word(r, &high, address + 2);
    *value = (uint32_t)low | ((uint32_t)high << 16);
    return 1;
}

int ram_read(RAMDev *r, uint32_t *value, uint32_t address, size_t width)
{
    RAMDev *ram = r;
    if (address > RAM_SIZE - 4) { return 0; }
    if (width == 8)
        return ram_read_byte(ram, (uint8_t *)value, address);
    else if (width == 16)
        return ram_read_word(ram, (uint16_t *)value, address);
    else if (width == 32)
        return ram_read_dword(ram, value, address);
    return 1;
}

int ram_write(RAMDev *r, uint32_t value, uint32_t address, size_t width)
{
    RAMDev *ram = r;

    if (address > RAM_SIZE - 4) { return 0; }
    if (width == 8) 
        return ram_write_byte(r, (uint8_t)value, address);
    else if (width == 16)
        return ram_write_word(r, (uint16_t)value, address);
    else if (width == 32)
        return ram_write_dword(r, value, address);

    return 1;
}

int console_read_stub(void *device, uint32_t *value, uint32_t address, size_t width)
{
    (void)device; (void)value; address = 0; width = 0;
    return 1;
}

int console_write(void *device, uint32_t data, uint32_t addr, size_t width) 
{
    // call ui
    return 1;
}

static inline uint32_t offset_to_cell_index(uint32_t offset)
{
    return offset /2;
}

coord find_cell(uint32_t cell_index) 
{
    uint32_t x = cell_index % SCREEN_WIDTH;
    uint32_t y = cell_index / SCREEN_WIDTH;
    return (coord){x, y};
}


int vram_write(void *device, uint32_t data, uint32_t addr, size_t width)
{
    //mov of 32 bit will be truncated to 16 bits. there are no 16 bit MOV implementations, so we treat mov 32 as if it were width 16
    if (!(((width == 8) && (addr < VGA_BASE_ADDR + VGA_SIZE) && (addr >= VGA_BASE_ADDR)) ||
        ((width == 32) && (addr + 1 < VGA_BASE_ADDR + VGA_SIZE) && (addr >= VGA_BASE_ADDR)))) { return 0; }

    uint32_t offset = addr - VGA_BASE_ADDR;
    uint32_t cell_index = offset_to_cell_index(offset);
    coord c = find_cell(cell_index);

    if (c.y >= SCREEN_HEIGHT || c.x >= SCREEN_WIDTH) { return 0; }
    VGADev *v = device;
    uint16_t cell = v->vram[c.y][c.x];
    bool changed = true;

    if (width == 8)
    {
        if ((offset % 2) == 0) 
            cell = (cell & 0xFF00u) | (uint16_t)data; //char to write
        else 
            cell = (cell & 0x00FFu) | ((uint16_t)data << 8); //atribute
    }
    else if (width == 32 && (offset % 2) == 0)  // word should not start at an odd byte or the attr byte would overwrite another cell's char byte
    {
        cell = (uint16_t)data;
    }
    else changed = false;

    if (changed)
    {
        v->vram[c.y][c.x] = cell;
        if (v->dirty_map[cell_index] == false && v->counter_vram_changes < SCREEN_HEIGHT*SCREEN_WIDTH)
        {
            v->dirty_map[cell_index] = true;
            v->dirty[v->counter_vram_changes++] = cell_index;
        }
        return 1;
    }
    return 0;
}

int vram_read_stub(void *device, uint32_t *value, uint32_t address, size_t width)
{
    (void)device; (void)value; address = 0; width = 0;
    return 1;
}
