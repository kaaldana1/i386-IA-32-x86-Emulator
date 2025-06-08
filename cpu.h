#ifndef CPU_H
#define CPU_H


#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define DEBUG
#define LITTLE_ENDIAN_SYS

#ifdef LITTLE_ENDIAN_SYS
#define DL 0
#define DH 1
#define AL 0
#define AH 1
#define BL 0
#define BH 1
#define CL 0
#define CH 1
#endif

#ifdef BIG_ENDIAN_SYS 
#define DL 1
#define DH 0
#define AL 1
#define AH 0
#define BL 1
#define BH 0
#define CL 1
#define CH 0
#endif

// GENERAL PUPROSE REGISTERS
#define GPR_AMOUNT 9
#define BIT_MODE_16 16
#define BIT_MODE_32 32

/*
#define DL (0xFF & EDX)
#define DH ((EDX >> 8) & 0xFF)
#define AL (0xFF & EAX)
#define AH ((EAX >> 8) & 0xFF)
#define BL (0xFF & EBX)
#define BH ((EBX >> 8) & 0xFF)
#define CL (0xFF & ECX)
#define CH ((ECX >> 8) & 0xFF)
*/

typedef enum {
 EAX, EBX, ECX, ESP,
 EBP, EDI, ESI, EDX,
 EIP
} Register_type;
typedef union {
    uint32_t dword;
    uint16_t word[2];
    uint8_t byte[4];
} Register;

Register registers[GPR_AMOUNT];

#endif