#include "ui/display_api.h"
#include "hardware_sim/devices_internal.h"
#include "hardware_sim/memmap.h"
#include "hardware_sim/bus.h"
#include "core/interrupt/interrupt_handlers.h"

typedef struct 
{
    VGADev *vga_dev;
    KeyboardDev *keyboard;
    BUS *bus;
} Devices; /*should be changed to be dynamic*/

Devices devices = {NULL, NULL};

void keyboard_handler();
void vga_handler();

void init_interrupt_handler(BUS *bus, VGADev *vga_dev, KeyboardDev *keyboard)
{
    devices.keyboard = keyboard;
    devices.vga_dev = vga_dev;
    devices.bus = bus;
}

IRQ_handler irq_handler_table[IRQ_COUNT] = {
    [KB_IRQ] = keyboard_handler,
    [VGA_IRQ] = vga_handler
};


void keyboard_handler()
{
    KeyboardDev *kb =  devices.keyboard;
    uint8_t data = 0;
    kb->dequeue(kb->keyboard_buffer, &data, &kb->keystrokes_in_queue, sizeof(kb->keyboard_buffer));

    bus_write(devices.bus, data, KEYBOARD_BASE_ADDR, 8);
}

void vga_handler()
{
    machine_state.ui_callbacks.ui_update_screen();
}


