#ifndef OPCODE_LIST_H
#define OPCODE_LIST_H

// Each X(name, opcode,
//         has_modrm, immediate_bytes, operand_count,
//         operand_type, opclass, width)
// matches Instruction_metadata fields.

#define FOREACH_OPCODE(X) \
/* 00–0F ------------------------------------------------ */ \
X(ADD_RM8_R8,       0x00, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_ADD,    8   ) \
X(ADD_RM32_R32,     0x01, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_ADD,    32  ) \
X(ADD_R8_RM8,       0x02, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_ADD,    8   ) \
X(ADD_R32_RM32,     0x03, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_ADD,    32  ) \
X(ADD_AL_IMM8,      0x04, NO_MODRM,       SINGLE_BYTE_IMM,    2, IMM_TO_REG,    OPC_ADD,    8   ) \
X(ADD_EAX_IMM32,    0x05, NO_MODRM,       FOUR_BYTE_IMM,      2, IMM_TO_REG,    OPC_ADD,    32  ) \
X(PUSH_ES,          0x06, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_PUSH,   0   ) \
X(POP_ES,           0x07, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_POP,    0   ) \
X(OR_RM8_R8,        0x08, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_OR,     8   ) \
X(OR_RM32_R32,      0x09, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_OR,     32  ) \
X(OR_R8_RM8,        0x0A, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_OR,     8   ) \
X(OR_R32_RM32,      0x0B, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_OR,     32  ) \
X(OR_AL_IMM8,       0x0C, NO_MODRM,       SINGLE_BYTE_IMM,    2, IMM_TO_REG,    OPC_OR,     8   ) \
X(OR_EAX_IMM32,     0x0D, NO_MODRM,       FOUR_BYTE_IMM,      2, IMM_TO_REG,    OPC_OR,     32  ) \
X(PUSH_CS,          0x0E, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_PUSH,   0   ) \
X(TWO_BYTE_ESC,     0x0F, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_ESC,    0   ) \
/* 10–1F (ADC) ---------------------------------------- */ \
X(ADC_RM8_R8,       0x10, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_ADC,    8   ) \
X(ADC_RM32_R32,     0x11, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_ADC,    32  ) \
X(ADC_R8_RM8,       0x12, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_ADC,    8   ) \
X(ADC_R32_RM32,     0x13, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_ADC,    32  ) \
X(ADC_AL_IMM8,      0x14, NO_MODRM,       SINGLE_BYTE_IMM,    2, IMM_TO_REG,    OPC_ADC,    8   ) \
X(ADC_EAX_IMM32,    0x15, NO_MODRM,       FOUR_BYTE_IMM,      2, IMM_TO_REG,    OPC_ADC,    32  ) \
X(PUSH_SS,          0x16, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_PUSH,   0   ) \
X(POP_SS,           0x17, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_POP,    0   ) \
X(SBB_RM8_R8,       0x18, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_SBB,    8   ) \
X(SBB_RM32_R32,     0x19, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_SBB,    32  ) \
X(SBB_R8_RM8,       0x1A, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_SBB,    8   ) \
X(SBB_R32_RM32,     0x1B, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_SBB,    32  ) \
X(SBB_AL_IMM8,      0x1C, NO_MODRM,       SINGLE_BYTE_IMM,    2, IMM_TO_REG,    OPC_SBB,    8   ) \
X(SBB_EAX_IMM32,    0x1D, NO_MODRM,       FOUR_BYTE_IMM,      2, IMM_TO_REG,    OPC_SBB,    32  ) \
X(PUSH_DS,          0x1E, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_PUSH,   0   ) \
X(POP_DS,           0x1F, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_POP,    0   ) \
/* 20–2F (AND/DAA/DAS, prefixes) --------------------- */ \
X(AND_RM8_R8,       0x20, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_AND,    8   ) \
X(AND_RM32_R32,     0x21, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_AND,    32  ) \
X(AND_R8_RM8,       0x22, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_AND,    8   ) \
X(AND_R32_RM32,     0x23, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_AND,    32  ) \
X(AND_AL_IMM8,      0x24, NO_MODRM,       SINGLE_BYTE_IMM,    2, IMM_TO_REG,    OPC_AND,    8   ) \
X(AND_EAX_IMM32,    0x25, NO_MODRM,       FOUR_BYTE_IMM,      2, IMM_TO_REG,    OPC_AND,    32  ) \
X(SEG_ES,           0x26, NO_MODRM,       NO_IMMEDIATE,       0, PREFIX,        OPC_PREFIX, 0   ) \
X(DAA,              0x27, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_DAA,    0   ) \
X(SUB_RM8_R8,       0x28, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_SUB,    8   ) \
X(SUB_RM32_R32,     0x29, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_SUB,    32  ) \
X(SUB_R8_RM8,       0x2A, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_SUB,    8   ) \
X(SUB_R32_RM32,     0x2B, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_SUB,    32  ) \
X(SUB_AL_IMM8,      0x2C, NO_MODRM,       SINGLE_BYTE_IMM,    2, IMM_TO_REG,    OPC_SUB,    8   ) \
X(SUB_EAX_IMM32,    0x2D, NO_MODRM,       FOUR_BYTE_IMM,      2, IMM_TO_REG,    OPC_SUB,    32  ) \
X(SEG_CS,           0x2E, NO_MODRM,       NO_IMMEDIATE,       0, PREFIX,        OPC_PREFIX, 0   ) \
X(DAS,              0x2F, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_DAS,    0   ) \
/* 30–3F (XOR/CMP/NOP) -------------------------------- */ \
X(XOR_RM8_R8,       0x30, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_XOR,    8   ) \
X(XOR_RM32_R32,     0x31, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_XOR,    32  ) \
X(XOR_R8_RM8,       0x32, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_XOR,    8   ) \
X(XOR_R32_RM32,     0x33, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_XOR,    32  ) \
X(XOR_AL_IMM8,      0x34, NO_MODRM,       SINGLE_BYTE_IMM,    2, IMM_TO_REG,    OPC_XOR,    8   ) \
X(XOR_EAX_IMM32,    0x35, NO_MODRM,       FOUR_BYTE_IMM,      2, IMM_TO_REG,    OPC_XOR,    32  ) \
X(SEG_SS,           0x36, NO_MODRM,       NO_IMMEDIATE,       0, PREFIX,        OPC_PREFIX, 0   ) \
X(AAA,              0x37, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_AAA,    0   ) \
X(CMP_RM8_R8,       0x38, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_CMP,    8   ) \
X(CMP_RM32_R32,     0x39, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_CMP,    32  ) \
X(CMP_R8_RM8,       0x3A, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_CMP,    8   ) \
X(CMP_R32_RM32,     0x3B, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_CMP,    32  ) \
X(CMP_AL_IMM8,      0x3C, NO_MODRM,       SINGLE_BYTE_IMM,    2, IMM_TO_REG,    OPC_CMP,    8   ) \
X(CMP_EAX_IMM32,    0x3D, NO_MODRM,       FOUR_BYTE_IMM,      2, IMM_TO_REG,    OPC_CMP,    32  ) \
X(SEG_DS,           0x3E, NO_MODRM,       NO_IMMEDIATE,       0, PREFIX,        OPC_PREFIX, 0   ) \
X(AAS,              0x3F, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_AAS,    0   ) \
/* 40–4F (INC/DEC r32) -------------------------------- */ \
X(INC_EAX,          0x40, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_INC,    32  ) \
X(INC_ECX,          0x41, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_INC,    32  ) \
X(INC_EDX,          0x42, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_INC,    32  ) \
X(INC_EBX,          0x43, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_INC,    32  ) \
X(INC_ESP,          0x44, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_INC,    32  ) \
X(INC_EBP,          0x45, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_INC,    32  ) \
X(INC_ESI,          0x46, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_INC,    32  ) \
X(INC_EDI,          0x47, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_INC,    32  ) \
X(DEC_EAX,          0x48, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_DEC,    32  ) \
X(DEC_ECX,          0x49, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_DEC,    32  ) \
X(DEC_EDX,          0x4A, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_DEC,    32  ) \
X(DEC_EBX,          0x4B, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_DEC,    32  ) \
X(DEC_ESP,          0x4C, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_DEC,    32  ) \
X(DEC_EBP,          0x4D, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_DEC,    32  ) \
X(DEC_ESI,          0x4E, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_DEC,    32  ) \
X(DEC_EDI,          0x4F, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_DEC,    32  ) \
/* 50–5F (PUSH/POP r32) -------------------------------- */ \
X(PUSH_EAX,         0x50, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_PUSH,   32  ) \
X(PUSH_ECX,         0x51, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_PUSH,   32  ) \
X(PUSH_EDX,         0x52, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_PUSH,   32  ) \
X(PUSH_EBX,         0x53, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_PUSH,   32  ) \
X(PUSH_ESP,         0x54, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_PUSH,   32  ) \
X(PUSH_EBP,         0x55, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_PUSH,   32  ) \
X(PUSH_ESI,         0x56, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_PUSH,   32  ) \
X(PUSH_EDI,         0x57, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_PUSH,   32  ) \
X(POP_EAX,          0x58, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_POP,    32  ) \
X(POP_ECX,          0x59, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_POP,    32  ) \
X(POP_EDX,          0x5A, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_POP,    32  ) \
X(POP_EBX,          0x5B, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_POP,    32  ) \
X(POP_ESP,          0x5C, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_POP,    32  ) \
X(POP_EBP,          0x5D, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_POP,    32  ) \
X(POP_ESI,          0x5E, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_POP,    32  ) \
X(POP_EDI,          0x5F, NO_MODRM,       NO_IMMEDIATE,       1, REG_ONLY,      OPC_POP,    32  ) \
/* 60–6F ------------------------------------------------ */ \
X(PUSHA,            0x60, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_PUSHA,  0   ) \
X(POPA,             0x61, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_POPA,   0   ) \
X(BOUND_GV_MA,      0x62, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_BOUND,  0   ) \
X(ARPL,             0x63, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_ARPL,   0   ) \
X(SEG_FS,           0x64, NO_MODRM,       NO_IMMEDIATE,       0, PREFIX,        OPC_PREFIX, 0   ) \
X(SEG_GS,           0x65, NO_MODRM,       NO_IMMEDIATE,       0, PREFIX,        OPC_PREFIX, 0   ) \
X(OPERAND_SIZE,     0x66, NO_MODRM,       NO_IMMEDIATE,       0, PREFIX,        OPC_PREFIX, 0   ) \
X(ADDRESS_SIZE,     0x67, NO_MODRM,       NO_IMMEDIATE,       0, PREFIX,        OPC_PREFIX, 0   ) \
X(PUSH_IMM32,       0x68, NO_MODRM,       FOUR_BYTE_IMM,      1, IMM_ONLY,      OPC_PUSH,   32  ) \
X(IMUL_GV_EV_IV32,  0x69, HAS_MODRM,      FOUR_BYTE_IMM,      3, TERNARY,       OPC_IMUL,   32  ) \
X(PUSH_IMM8,        0x6A, NO_MODRM,       SINGLE_BYTE_IMM,    1, IMM_ONLY,      OPC_PUSH,   8   ) \
X(IMUL_GV_EV_IB,    0x6B, HAS_MODRM,      SINGLE_BYTE_IMM,    3, TERNARY,       OPC_IMUL,   32  ) \
X(INSB,             0x6C, NO_MODRM,       NO_IMMEDIATE,       0, STR_OP,        OPC_INS,    8   ) \
X(INSD,             0x6D, NO_MODRM,       NO_IMMEDIATE,       0, STR_OP,        OPC_INS,    32  ) \
X(OUTSB,            0x6E, NO_MODRM,       NO_IMMEDIATE,       0, STR_OP,        OPC_OUTS,   8   ) \
X(OUTSD,            0x6F, NO_MODRM,       NO_IMMEDIATE,       0, STR_OP,        OPC_OUTS,   32  ) \
/* 70–7F (Jcc rel8) ----------------------------------- */ \
X(JO_REL8,          0x70, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_JCC,    8   ) \
X(JNO_REL8,         0x71, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_JCC,    8   ) \
X(JB_REL8,          0x72, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_JCC,    8   ) \
X(JAE_REL8,         0x73, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_JCC,    8   ) \
X(JE_REL8,          0x74, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_JCC,    8   ) \
X(JNE_REL8,         0x75, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_JCC,    8   ) \
X(JBE_REL8,         0x76, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_JCC,    8   ) \
X(JA_REL8,          0x77, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_JCC,    8   ) \
X(JS_REL8,          0x78, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_JCC,    8   ) \
X(JNS_REL8,         0x79, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_JCC,    8   ) \
X(JP_REL8,          0x7A, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_JCC,    8   ) \
X(JNP_REL8,         0x7B, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_JCC,    8   ) \
X(JL_REL8,          0x7C, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_JCC,    8   ) \
X(JGE_REL8,         0x7D, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_JCC,    8   ) \
X(JLE_REL8,         0x7E, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_JCC,    8   ) \
X(JG_REL8,          0x7F, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_JCC,    8   ) \
/* 80–8F ------------------------------------------------ */ \
X(IMM_GRP_EB_LB,    0x80, HAS_MODRM,      SINGLE_BYTE_IMM,    2, GRP1_IMM,      OPC_GRP1,   8   ) \
X(IMM_GRP_EV_LZ,    0x81, HAS_MODRM,      FOUR_BYTE_IMM,      2, GRP1_IMM,      OPC_GRP1,   32  ) \
X(IMM_GRP_EV_LB,    0x83, HAS_MODRM,      SINGLE_BYTE_IMM,    2, GRP1_IMM,      OPC_GRP1,   32  ) \
X(TEST_RM8_R8,      0x84, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_TEST,   8   ) \
X(TEST_RM32_R32,    0x85, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_TEST,   32  ) \
X(XCHG_RM8_R8,      0x86, HAS_MODRM,      NO_IMMEDIATE,       2, BIDIR,         OPC_XCHG,   8   ) \
X(XCHG_RM32_R32,    0x87, HAS_MODRM,      NO_IMMEDIATE,       2, BIDIR,         OPC_XCHG,   32  ) \
X(MOV_RM8_R8,       0x88, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_MOV,    8   ) \
X(MOV_RM32_R32,     0x89, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_MOV,    32  ) \
X(MOV_R8_RM8,       0x8A, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_MOV,    8   ) \
X(MOV_R32_RM32,     0x8B, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_MOV,    32  ) \
X(MOV_SREG_RM16,    0x8C, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_MOV,    16  ) \
X(LEA,              0x8D, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_LEA,    32  ) \
X(MOV_RM16_SREG,    0x8E, HAS_MODRM,      NO_IMMEDIATE,       2, REG_TO_MEM,    OPC_MOV,    16  ) \
X(POP_RM32,         0x8F, HAS_MODRM,      NO_IMMEDIATE,       1, MEM_ONLY,      OPC_POP,    32  ) \
/* 90–9F ------------------------------------------------ */ \
X(NOP,              0x90, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_NOP,    0   ) \
X(XCHG_ECX_EAX,     0x91, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_XCHG,   32  ) \
X(XCHG_EDX_EAX,     0x92, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_XCHG,   32  ) \
X(XCHG_EBX_EAX,     0x93, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_XCHG,   32  ) \
X(XCHG_ESP_EAX,     0x94, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_XCHG,   32  ) \
X(XCHG_EBP_EAX,     0x95, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_XCHG,   32  ) \
X(XCHG_ESI_EAX,     0x96, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_XCHG,   32  ) \
X(XCHG_EDI_EAX,     0x97, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_XCHG,   32  ) \
X(CBW_CWDE,         0x98, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_CBW,    32  ) \
X(CWD_CDQ,          0x99, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_CWD,    32  ) \
X(CALL_FAR_PTR,     0x9A, NO_MODRM,       SIX_BYTE_IMM,       1, IMM_ONLY,      OPC_CALL,   0   ) \
X(WAIT_FWAIT,       0x9B, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_WAIT,   0   ) \
X(PUSHF,            0x9C, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_PUSHF,  32  ) \
X(POPF,             0x9D, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_POPF,   32  ) \
X(SAHF,             0x9E, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_SAHF,   8   ) \
X(LAHF,             0x9F, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_LAHF,   8   ) \
/* A0–AF (MOV moffs / string MOVS) -------------------- */ \
X(MOV_AL_MOFFS8,    0xA0, NO_MODRM,       FOUR_BYTE_IMM,      2, MEM_TO_REG,    OPC_MOV,    8   ) \
X(MOV_EAX_MOFFS32,  0xA1, NO_MODRM,       FOUR_BYTE_IMM,      2, MEM_TO_REG,    OPC_MOV,    32  ) \
X(MOV_MOFFS8_AL,    0xA2, NO_MODRM,       FOUR_BYTE_IMM,      2, REG_TO_MEM,    OPC_MOV,    8   ) \
X(MOV_MOFFS32_EAX,  0xA3, NO_MODRM,       FOUR_BYTE_IMM,      2, REG_TO_MEM,    OPC_MOV,    32  ) \
X(MOVSB,            0xA4, NO_MODRM,       NO_IMMEDIATE,       0, STR_OP,        OPC_MOVS,   8   ) \
X(MOVSD,            0xA5, NO_MODRM,       NO_IMMEDIATE,       0, STR_OP,        OPC_MOVS,   32  ) \
X(CMPSB,            0xA6, NO_MODRM,       NO_IMMEDIATE,       0, STR_OP,        OPC_CMPS,   8   ) \
X(CMPSD,            0xA7, NO_MODRM,       NO_IMMEDIATE,       0, STR_OP,        OPC_CMPS,   32  ) \
X(TEST_AL_IMM8,     0xA8, NO_MODRM,       SINGLE_BYTE_IMM,    2, IMM_TO_REG,    OPC_TEST,   8   ) \
X(TEST_EAX_IMM32,   0xA9, NO_MODRM,       FOUR_BYTE_IMM,      2, IMM_TO_REG,    OPC_TEST,   32  ) \
X(STOSB,            0xAA, NO_MODRM,       NO_IMMEDIATE,       0, STR_OP,        OPC_STOS,   8   ) \
X(STOSD,            0xAB, NO_MODRM,       NO_IMMEDIATE,       0, STR_OP,        OPC_STOS,   32  ) \
X(LODSB,            0xAC, NO_MODRM,       NO_IMMEDIATE,       0, STR_OP,        OPC_LODS,   8   ) \
X(LODSD,            0xAD, NO_MODRM,       NO_IMMEDIATE,       0, STR_OP,        OPC_LODS,   32  ) \
X(SCASB,            0xAE, NO_MODRM,       NO_IMMEDIATE,       0, STR_OP,        OPC_SCAS,   8   ) \
X(SCASD,            0xAF, NO_MODRM,       NO_IMMEDIATE,       0, STR_OP,        OPC_SCAS,   32  ) \
/* B0–BF (MOV r8/rv, imm) ----------------------------- */ \
X(MOV_AL_IMM8,      0xB0, NO_MODRM,       SINGLE_BYTE_IMM,    2, IMM_TO_REG,    OPC_MOV,    8   ) \
X(MOV_CL_IMM8,      0xB1, NO_MODRM,       SINGLE_BYTE_IMM,    2, IMM_TO_REG,    OPC_MOV,    8   ) \
X(MOV_DL_IMM8,      0xB2, NO_MODRM,       SINGLE_BYTE_IMM,    2, IMM_TO_REG,    OPC_MOV,    8   ) \
X(MOV_BL_IMM8,      0xB3, NO_MODRM,       SINGLE_BYTE_IMM,    2, IMM_TO_REG,    OPC_MOV,    8   ) \
X(MOV_AH_IMM8,      0xB4, NO_MODRM,       SINGLE_BYTE_IMM,    2, IMM_TO_REG,    OPC_MOV,    8   ) \
X(MOV_CH_IMM8,      0xB5, NO_MODRM,       SINGLE_BYTE_IMM,    2, IMM_TO_REG,    OPC_MOV,    8   ) \
X(MOV_DH_IMM8,      0xB6, NO_MODRM,       SINGLE_BYTE_IMM,    2, IMM_TO_REG,    OPC_MOV,    8   ) \
X(MOV_BH_IMM8,      0xB7, NO_MODRM,       SINGLE_BYTE_IMM,    2, IMM_TO_REG,    OPC_MOV,    8   ) \
X(MOV_EAX_IMM32,    0xB8, NO_MODRM,       FOUR_BYTE_IMM,      2, IMM_TO_REG,    OPC_MOV,    32  ) \
X(MOV_ECX_IMM32,    0xB9, NO_MODRM,       FOUR_BYTE_IMM,      2, IMM_TO_REG,    OPC_MOV,    32  ) \
X(MOV_EDX_IMM32,    0xBA, NO_MODRM,       FOUR_BYTE_IMM,      2, IMM_TO_REG,    OPC_MOV,    32  ) \
X(MOV_EBX_IMM32,    0xBB, NO_MODRM,       FOUR_BYTE_IMM,      2, IMM_TO_REG,    OPC_MOV,    32  ) \
X(MOV_ESP_IMM32,    0xBC, NO_MODRM,       FOUR_BYTE_IMM,      2, IMM_TO_REG,    OPC_MOV,    32  ) \
X(MOV_EBP_IMM32,    0xBD, NO_MODRM,       FOUR_BYTE_IMM,      2, IMM_TO_REG,    OPC_MOV,    32  ) \
X(MOV_ESI_IMM32,    0xBE, NO_MODRM,       FOUR_BYTE_IMM,      2, IMM_TO_REG,    OPC_MOV,    32  ) \
X(MOV_EDI_IMM32,    0xBF, NO_MODRM,       FOUR_BYTE_IMM,      2, IMM_TO_REG,    OPC_MOV,    32  ) \
/* C0–CF ---------------------------------------------- */ \
X(GRP2_EB_IB,       0xC0, HAS_MODRM,      SINGLE_BYTE_IMM,    2, GRP2_SHIFT,    OPC_GRP2,   8   ) \
X(GRP2_EV_IB,       0xC1, HAS_MODRM,      SINGLE_BYTE_IMM,    2, GRP2_SHIFT,    OPC_GRP2,   32  ) \
X(RET_NEAR_IMM16,   0xC2, NO_MODRM,       TWO_BYTE_IMM,       1, IMM_ONLY,      OPC_RET,    0   ) \
X(RET_NEAR,         0xC3, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_RET,    0   ) \
X(LES,              0xC4, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_LES,    0   ) \
X(LDS,              0xC5, HAS_MODRM,      NO_IMMEDIATE,       2, MEM_TO_REG,    OPC_LDS,    0   ) \
X(MOV_EB_IB,        0xC6, HAS_MODRM,      SINGLE_BYTE_IMM,    2, IMM_TO_MEM,    OPC_MOV,    8   ) \
X(MOV_EV_IV,        0xC7, HAS_MODRM,      FOUR_BYTE_IMM,      2, IMM_TO_MEM,    OPC_MOV,    32  ) \
X(ENTER,            0xC8, NO_MODRM,       FOUR_BYTE_IMM,      2, IMM_ONLY,      OPC_ENTER,  0   ) \
X(LEAVE,            0xC9, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_LEAVE,  0   ) \
X(RET_FAR_IMM16,    0xCA, NO_MODRM,       TWO_BYTE_IMM,       1, IMM_ONLY,      OPC_RETF,   0   ) \
X(RET_FAR,          0xCB, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_RETF,   0   ) \
X(INT3,             0xCC, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_INT3,   0   ) \
X(INT_IB,           0xCD, NO_MODRM,       SINGLE_BYTE_IMM,    1, IMM_ONLY,      OPC_INT,    0   ) \
X(INTO,             0xCE, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_INTO,   0   ) \
X(IRET,             0xCF, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_IRET,   0   ) \
/* D0–DF (shift/rotate groups, FPU map) --------------- */ \
X(SHIFT_EB_1,       0xD0, HAS_MODRM,      NO_IMMEDIATE,       2, GRP2_SHIFT,    OPC_GRP2,   8   ) \
X(SHIFT_EV_1,       0xD1, HAS_MODRM,      NO_IMMEDIATE,       2, GRP2_SHIFT,    OPC_GRP2,   32  ) \
X(SHIFT_EB_CL,      0xD2, HAS_MODRM,      NO_IMMEDIATE,       2, GRP2_SHIFT,    OPC_GRP2,   8   ) \
X(SHIFT_EV_CL,      0xD3, HAS_MODRM,      NO_IMMEDIATE,       2, GRP2_SHIFT,    OPC_GRP2,   32  ) \
X(FPU_ESC_D8,       0xD8, HAS_MODRM,      NO_IMMEDIATE,       0, FPU,           OPC_FPU,    0   ) \
X(FPU_ESC_D9,       0xD9, HAS_MODRM,      NO_IMMEDIATE,       0, FPU,           OPC_FPU,    0   ) \
X(FPU_ESC_DA,       0xDA, HAS_MODRM,      NO_IMMEDIATE,       0, FPU,           OPC_FPU,    0   ) \
X(FPU_ESC_DB,       0xDB, HAS_MODRM,      NO_IMMEDIATE,       0, FPU,           OPC_FPU,    0   ) \
X(FPU_ESC_DC,       0xDC, HAS_MODRM,      NO_IMMEDIATE,       0, FPU,           OPC_FPU,    0   ) \
X(FPU_ESC_DD,       0xDD, HAS_MODRM,      NO_IMMEDIATE,       0, FPU,           OPC_FPU,    0   ) \
X(FPU_ESC_DE,       0xDE, HAS_MODRM,      NO_IMMEDIATE,       0, FPU,           OPC_FPU,    0   ) \
X(FPU_ESC_DF,       0xDF, HAS_MODRM,      NO_IMMEDIATE,       0, FPU,           OPC_FPU,    0   ) \
/* E0–EF ------------------------------------------------ */ \
X(LOOPNE_REL8,      0xE0, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_LOOPCC, 8   ) \
X(LOOPE_REL8,       0xE1, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_LOOPCC, 8   ) \
X(LOOP_REL8,        0xE2, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_LOOP,   8   ) \
X(JECXZ_REL8,       0xE3, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_JECXZ,  8   ) \
X(IN_AL_IB,         0xE4, NO_MODRM,       SINGLE_BYTE_IMM,    1, PORT_IO,       OPC_IN,     8   ) \
X(IN_EAX_IB,        0xE5, NO_MODRM,       SINGLE_BYTE_IMM,    1, PORT_IO,       OPC_IN,     32  ) \
X(OUT_IB_AL,        0xE6, NO_MODRM,       SINGLE_BYTE_IMM,    1, PORT_IO,       OPC_OUT,    8   ) \
X(OUT_IB_EAX,       0xE7, NO_MODRM,       SINGLE_BYTE_IMM,    1, PORT_IO,       OPC_OUT,    32  ) \
X(CALL_REL32,       0xE8, NO_MODRM,       FOUR_BYTE_IMM,      1, RELATIVE,      OPC_CALL,   32  ) \
X(JMP_REL32,        0xE9, NO_MODRM,       FOUR_BYTE_IMM,      1, RELATIVE,      OPC_JMP,    32  ) \
X(JMP_PTR16_32,     0xEA, NO_MODRM,       SIX_BYTE_IMM,       1, IMM_ONLY,      OPC_JMP,    0   ) \
X(JMP_REL8,         0xEB, NO_MODRM,       SINGLE_BYTE_IMM,    1, RELATIVE,      OPC_JMP,    8   ) \
X(IN_AL_DX,         0xEC, NO_MODRM,       NO_IMMEDIATE,       1, PORT_IO,       OPC_IN,     8   ) \
X(IN_EAX_DX,        0xED, NO_MODRM,       NO_IMMEDIATE,       1, PORT_IO,       OPC_IN,     32  ) \
X(OUT_DX_AL,        0xEE, NO_MODRM,       NO_IMMEDIATE,       1, PORT_IO,       OPC_OUT,    8   ) \
X(OUT_DX_EAX,       0xEF, NO_MODRM,       NO_IMMEDIATE,       1, PORT_IO,       OPC_OUT,    32  ) \
/* F0–FF ------------------------------------------------ */ \
X(LOCK,             0xF0, NO_MODRM,       NO_IMMEDIATE,       0, PREFIX,        OPC_PREFIX, 0   ) \
X(INT1,             0xF1, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_INT1,   0   ) \
X(REPNE,            0xF2, NO_MODRM,       NO_IMMEDIATE,       0, PREFIX,        OPC_PREFIX, 0   ) \
X(REPE,             0xF3, NO_MODRM,       NO_IMMEDIATE,       0, PREFIX,        OPC_PREFIX, 0   ) \
X(HLT,              0xF4, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_HLT,    0   ) \
X(CMC,              0xF5, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_CMC,    0   ) \
X(GRP3_EB,          0xF6, HAS_MODRM,      NO_IMMEDIATE,       0, GRP3_MISC,     OPC_GRP3,   8   ) \
X(GRP3_EV,          0xF7, HAS_MODRM,      NO_IMMEDIATE,       0, GRP3_MISC,     OPC_GRP3,   32  ) \
X(CLC,              0xF8, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_CLC,    0   ) \
X(STC,              0xF9, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_STC,    0   ) \
X(CLI,              0xFA, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_CLI,    0   ) \
X(STI,              0xFB, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_STI,    0   ) \
X(CLD,              0xFC, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_CLD,    0   ) \
X(STD,              0xFD, NO_MODRM,       NO_IMMEDIATE,       0, NO_OPERANDS,   OPC_STD,    0   ) \
X(GRP4,             0xFE, HAS_MODRM,      NO_IMMEDIATE,       0, GRP4_INCDEC,   OPC_GRP4,   8   ) \
X(GRP5,             0xFF, HAS_MODRM,      NO_IMMEDIATE,       0, GRP5_MISC,     OPC_GRP5,   32  )

typedef enum {
#define X(name, _1, _2, _3, _4, _5, _6, _7) name,
    FOREACH_OPCODE(X)
#undef X
OPCODE_COUNT
} Opcode_ID;

#endif // OPCODE_LIST_H
