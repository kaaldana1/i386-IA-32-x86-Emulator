#ifndef REGISTER_IDS
#define REGISTER_IDS

typedef enum
{
    EAX, ECX, EDX, EBX, EIP,
    EBP, ESI, EDI, ESP
} GeneralPurposeRegisterType;

typedef enum 
{
    CS, DS, ES, FS, GS, SS
} SegmentRegisterType;

typedef enum
{
     SF = 1 << 0,  /*sign flag: 
            SF = 1 if the MSB of a result is 1, */
     ZF = 1 << 1, /*zero flag
            ZF = 1 if result is zero*/
     CF = 1 << 2, /*carry flag
            CF = 1 if: 
                    1. There is a carry put from MBS on addition
                    2. There is a borrow into the MSB on subtraction*/
     AF = 1 << 3, /*auxiliary carry flag
            AF = 1 if:
                    1. There is a carry out from bit 3 on addtion
                    2. Borrow into bit 3 on subtraction*/
     PF = 1 << 4, /*parity flag
            PF = 1 sif the low byte of a result has an event number*/
     OF = 1 << 5 /*overflow flag
            OF = 1 if signed overflow occured*/
} StatusFlagType;

typedef enum  
{
    DF = 1 << 6, /*directional flag*/
    IF = 1 << 7, /*interrupt flag*/
    TF = 1 << 8 /*trap flag*/
} ControlFlagType;

#endif
