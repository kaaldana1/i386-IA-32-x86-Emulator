#include <asmjit/a64.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <variant>
#include <vector>

extern "C" {
#include "core/structs/instruction.h"
#include "core/cpu_internal.h"
#include "tables/decode_tables.h"
}

namespace ia32 
{
    using Instruction = ::Instruction;
    using CPU = ::CPU;
    using OpcodeID = ::OpcodeID;
    using GuestReg = ::GeneralPurposeRegisterType;
    using InstructionMetadata = ::InstructionMetadata;
    using ::instr_metadata_lut;
    using ::single_byte_opcode_lut;

    constexpr OpcodeID accepted_ids[] = {
        OpcodeID::MOV_EAX_IMM32, OpcodeID::MOV_ECX_IMM32,
        OpcodeID::MOV_EDX_IMM32, OpcodeID::MOV_EBX_IMM32,
        OpcodeID::INC_EAX, OpcodeID::INC_ECX,
        OpcodeID::INC_EDX, OpcodeID::INC_EBX,
        OpcodeID::ADD_R32_RM32, OpcodeID::IMM_GRP_EV_LB,
        OpcodeID::CMP_R32_RM32, OpcodeID::JMP_REL8,
        OpcodeID::JMP_REL32, OpcodeID::JE_REL8, OpcodeID::JNE_REL8
    };

    constexpr bool is_accepted(OpcodeID opcode)
    {
        for (const OpcodeID accepted : accepted_ids) {
            if (opcode == accepted) return true;
        }
        return false;
    }
}

//============
// IR
//============
struct Value { std::uint32_t id; }; // Virtual register, not the host reg

struct Const32 { Value destination; std::uint32_t value; };
struct LoadReg32 { Value destination; ia32::GuestReg reg; };
struct StoreReg32 { ia32::GuestReg reg; Value source; };
struct Add32 { Value destination, lhs, rhs; };
struct IsZero32 { Value destination, source; };
struct Equal32 { Value destination, lhs, rhs; };
struct LoadZF { Value destination; };
struct StoreZF { Value source; };
struct Select32 { Value destination, condition, if_true, if_false; };
struct SetGuestEIP { std::uint32_t value; };
struct ExitBlock {};

using IR_object = std::variant<Const32, LoadReg32, StoreReg32, Add32, IsZero32,
                               Equal32, LoadZF, StoreZF, Select32, SetGuestEIP,
                               ExitBlock>;

using Block = std::vector<IR_object>;

enum class ARM64Mnemonic 
{
    MOVZ, 
    MOVK, 
    LDR, 
    STR, 
    ADD_IMMEDIATE, 
    ADD, 
    CMP, 
    CSET, 
    RET
};

//============
// JITruntime stuff
//============

using HostRegisterNumber = std::uint32_t;
using BlockFunction = void (*)(ia32::CPU* cpu_state);

// cpu_offset is a byte offset into CPU, not a guest address
asmjit::Error ARM64_emit(asmjit::a64::Assembler& assembler, ARM64Mnemonic mnemonic,
                                      HostRegisterNumber destination, HostRegisterNumber source,
                                      std::uint64_t immediate = 0, std::int32_t cpu_offset = 0, std::uint32_t shift = 0)
{
    namespace a64 = asmjit::a64;

    switch (mnemonic) {
        // MOVZ/MOVK take a 16-bit immediate and a shift of 0 or 16 for W regs
        case ARM64Mnemonic::MOVZ:
            return assembler.movz(a64::w(destination), immediate, shift);
        case ARM64Mnemonic::MOVK:
            return assembler.movk(a64::w(destination), immediate, shift);
        // LDR/STR assume x0 still holds the CPU pointer, so we preserve it before using w0 as scratch
        case ARM64Mnemonic::LDR:
            return assembler.ldr(a64::w(destination), a64::ptr(a64::x0, cpu_offset));
        case ARM64Mnemonic::STR:
            return assembler.str(a64::w(destination), a64::ptr(a64::x0, cpu_offset));
        case ARM64Mnemonic::ADD_IMMEDIATE:
            return assembler.add(a64::w(destination), a64::w(source), immediate);
        case ARM64Mnemonic::ADD:
            return assembler.add(a64::w(destination), a64::w(destination), a64::w(source));
        case ARM64Mnemonic::CMP:
            return assembler.cmp(a64::w(destination), immediate);
        case ARM64Mnemonic::CSET:
            return assembler.cset(a64::w(destination), asmjit::arm::CondCode::kEQ);
        case ARM64Mnemonic::RET:
            return assembler.ret(a64::x30);
        }
    return asmjit::Error::kInvalidInstruction;
}

enum class TranslationStatus 
{ 
    NotImplemented 
};

TranslationStatus translate(const ia32::Instruction* decoded_instructions, std::size_t instruction_count)
{
    // either IR or direct route
    return TranslationStatus::NotImplemented;
}

TranslationStatus direct_route(const ia32::Instruction& instruction, asmjit::a64::Assembler& assembler)
{
    return TranslationStatus::NotImplemented;
}

TranslationStatus IR_route(const ia32::Instruction& instruction, Block& block)
{
    return TranslationStatus::NotImplemented;
}

TranslationStatus code_install(asmjit::JitRuntime& runtime, asmjit::CodeHolder& code, BlockFunction& block_function)
{
    return TranslationStatus::NotImplemented;
}

//look up a block by guest EIP
BlockFunction cache_lookup(const ia32::CPU& cpu_state)
{
    return nullptr;
}

TranslationStatus dispatch(ia32::CPU& cpu_state, BlockFunction block_function)
{
    // lookup code in cache first, if not, itll will go through the decoder, translator
    // install code
    // dispatcher transfers control to the installed code
    return TranslationStatus::NotImplemented;
}

int main()
{
    asmjit::JitRuntime jit_runtime;
    asmjit::CodeHolder code;
    const asmjit::Error error = code.init(jit_runtime.environment());
    if (error != asmjit::Error::kOk) 
    {
        std::fprintf(stderr, "AsmJit initialization failed: %s\n", asmjit::stringify_error(error));
        return 1;
    }

    return 0;
}
