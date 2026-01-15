#ifndef PROGRAM_LOADER_H
#define PROGRAM_LOADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "hardware_sim/bus.h"


typedef struct {
    uint8_t *arr;
    size_t size;
    size_t capacity;
    FILE *filename;
} Program;
//hola
Program *create_program(char *filename);
int load_program(BUS *bus, Program *p, uint32_t code_addr);
void print_contents(Program *p);

#endif
