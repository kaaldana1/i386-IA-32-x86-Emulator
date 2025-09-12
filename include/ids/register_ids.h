#ifndef REGISTER_IDS
#define REGISTER_IDS

typedef enum
{
    EAX, ECX, EDX, EBX, EIP,
    EBP, ESI, EDI, ESP
} GPR_type;

typedef enum 
{
    CS, DS, ES, FS, GS, SS
} SR_type;

#endif
