#ifndef PROGRAM_LOADER_H 
#define PROGRAM_LOADER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t *program;
    size_t program_length;
} Program;

int parse_file(Program *p) {
    FILE *fptr = fopen("hex_code.txt", "rb");
    if ( fptr == NULL ) { printf("Lmao no"); exit(1); }

    char buff[200];
    uint8_t *tmp = (uint8_t*)malloc(sizeof(buff));
    if (tmp == NULL) { printf("Error"); exit(1); }

    int i;
    int each;
    size_t bytes = 0;
    int offset = 0;

    while (fgets(buff, sizeof(buff), fptr)) {
        i = 0;
        while (sscanf((buff + i), " %x%n", &each, &offset) == 1) {
            tmp[bytes++] = (uint8_t)each;
            i += offset;
        }
    }
    fclose(fptr);

    p->program = (uint8_t*)malloc(bytes);
    if (p->program == NULL) { printf("err"); exit(1); }

    memcpy(p->program, tmp, bytes);
    p->program_length = bytes;

    free(tmp);
    return 1;
}

void print_contents(Program *p) {
    if (p->program == NULL) { printf("Invalid"); exit(1); }

    for (int i = 0; i < p->program_length; i++) {
        printf("%02x ", p->program[i]);
    }
    printf("\nProgram size: %zu", p->program_length);
}

#endif