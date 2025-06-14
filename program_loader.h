#ifndef PROGRAM_LOADER_H
#define PROGRAM_LOADER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint8_t *program;
    size_t program_length;
} Program;

int parse_file(Program *p);
void print_contents(Program *p);

#endif