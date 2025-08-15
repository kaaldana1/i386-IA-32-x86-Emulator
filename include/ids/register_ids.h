#ifndef REGISTER_IDS
#define REGISTER_IDS

typedef enum
{
    EAX, EBX, ECX, ESP, EBP,
    EDI, ESI, EDX, EIP,
} GPR_type;

typedef enum 
{
    CS, DS, ES, FS, GS, SS
} SR_type;

#endif
