#ifndef OPCLASS_LIST_H
#define OPCLASS_LIST_H

#define FOR_EACH_ARITH_LOGIC_OP(X) \
    X(ADD,  0x01    )  \
    X(SUB,  0x02    )  \
    X(CMP,  0x04    )  \
    X(NEG,  0x08    )  \
    X(ADC,  0x10    )  \
    X(SBB,  0x20    )  \
    X(INC,  0x40    )  \
    X(DEC,  0x80    )  \
    X(AND,  0x100   )  \
    X(XOR,  0x200   )  \
    X(OR,   0x400   )  \
    X(TEST, 0x800   )  \
    X(SHL,  0x1000  )  \
    X(SHR,  0x2000  )  \
    X(SAR,  0x4000  )  \
    X(ROL,  0x8000  )  \
    X(ROR,  0x10000 )


#define FOR_EACH_GEN_OPCLASS(X) \
    X(AAS,        0x20000) \
    X(DAA,        0x20001) \
    X(DAS,        0x20002) \
    X(AAA,        0x20003) \
    X(PUSH,       0x20004) \
    X(PUSHA,      0x20005) \
    X(POP,        0x20006) \
    X(POPA,       0x20007) \
    X(BOUND,      0x20008) \
    X(ARPL,       0x20009) \
    X(IMUL,       0x2000A) \
    X(INS,        0x2000B) \
    X(OUTS,       0x2000C) \
    X(JCC,        0x2000D) \
    X(GRP1,       0x2000E) \
    X(XCHG,       0x2000F) \
    X(LEA,        0x20010) \
    X(CBW,        0x20011) \
    X(CALL,       0x20012) \
    X(WAIT,       0x20013) \
    X(PUSHF,      0x20014) \
    X(POPF,       0x20015) \
    X(SAHF,       0x20016) \
    X(LAHF,       0x20017) \
    X(MOVS,       0x20018) \
    X(STOS,       0x20019) \
    X(LODS,       0x2001A) \
    X(SCAS,       0x2001B) \
    X(ENTER,      0x2001C) \
    X(LEAVE,      0x2001D) \
    X(RETF,       0x2001E) \
    X(INT,        0x2001F) \
    X(INTO,       0x20020) \
    X(IRET,       0x20021) \
    X(FPU,        0x20022) \
    X(GRP2,       0x20023) \
    X(LOOPCC,     0x20024) \
    X(JECXZ,      0x20025) \
    X(IN,         0x20026) \
    X(OUT,        0x20027) \
    X(INT1,       0x20028) \
    X(HLT,        0x20029) \
    X(CMC,        0x2002A) \
    X(GRP3,       0x2002B) \
    X(CLC,        0x2002C) \
    X(STC,        0x2002D) \
    X(CLI,        0x2002E) \
    X(STI,        0x2002F) \
    X(CLD,        0x20030) \
    X(STD,        0x20031) \
    X(GRP4,       0x20032) \
    X(GRP5,       0x20033) \
    X(WCD,        0x20034) \
    X(CWD,        0x20035) \
    X(CDQ,        0x20036) \
    X(CMPS,       0x20037) \
    X(RET,        0x20038) \
    X(LES,        0x20039) \
    X(LDS,        0x2003A) \
    X(JMP,        0x2003B) \
    X(INT3,       0x2003C) \
    X(LOOP,       0x2003D) \
    X(MUL,        0x2003E) \
    X(DIV,        0x2003F) \
    X(IDIV,       0x20040) \
    X(BSF,        0x20041) \
    X(BSegmentRegister,        0x20042) \
    X(MOV,        0x20043) \
    X(NOP,        0x20044) \
    X(ESC,        0x20045) \
    X(PREFIX,     0x20046) \
    X(RCL,        0x20047) \
    X(RCR,        0x20048)


typedef enum 
{
    #define X(name, value) OPC_##name = value,
    FOR_EACH_ARITH_LOGIC_OP(X)
    #undef X

    #define X(name, value) OPC_##name = value,
    FOR_EACH_GEN_OPCLASS(X)
    #undef X

    OPCLASS_COUNT
} Opclass; 
#endif
