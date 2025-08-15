#ifndef PROGRAM_LOADER_H
#define PROGRAM_LOADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "hardware_sim/bus.h"

typedef struct Program Program;
//hola
Program *create_program(void);
int load_program(BUS *bus, Program *p, uint32_t code_addr);
void print_contents(Program *p);

#endif
