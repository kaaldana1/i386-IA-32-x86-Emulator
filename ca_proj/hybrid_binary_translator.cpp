#include <asmjit/a64.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <optional>
#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <variant>
#include <vector>
#include <unordered_map>

extern "C" {
#include "core/structs/instruction.h"
#include "core/cpu_internal.h"
#include "tables/decode_tables.h"
#include "core/decoder.h"
}

namespace ia32 
{

    using CPU = ::CPU;
    using GuestRegType = ::GeneralPurposeRegisterType;
    using GuestReg = :: GeneralPurposeRegister;

    using Instruction = ::Instruction;
    using OpcodeID = ::OpcodeID;
    using InstructionMetadata = ::InstructionMetadata;

    constexpr OpcodeID accepted_ids[] = {
        OpcodeID::MOV_EAX_IMM32, OpcodeID::MOV_ECX_IMM32,
        OpcodeID::MOV_EDX_IMM32, OpcodeID::MOV_EBX_IMM32,
        OpcodeID::INC_EAX, OpcodeID::INC_ECX,
        OpcodeID::INC_EDX, OpcodeID::INC_EBX,
        OpcodeID::ADD_R32_RM32, OpcodeID::IMM_GRP_EV_LB,
        OpcodeID::CMP_R32_RM32, OpcodeID::JMP_REL8,
        OpcodeID::JMP_REL32, OpcodeID::JE_REL8, OpcodeID::JNE_REL8
    };

    using ::instr_metadata_lut;
    using ::single_byte_opcode_lut;
    using ::decoder;

    using InstructionBundle = std::vector<Instruction*>;
    constexpr bool is_accepted(OpcodeID opcode)
    {
        for (const OpcodeID accepted : accepted_ids) {
            if (opcode == accepted) return true;
        }
        return false;
    }

    using GuestProgram = std::vector<std::uint8_t>;
    GuestProgram read_binary_file(const char* filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file) { throw std::runtime_error("Couldn't open bin"); }

        file.seekg(0, std::ios::end);
        const std::streamsize size = file.tellg();
        if (size < 0) { throw std::runtime_error("Could not determine file size"); }
        file.seekg(0, std::ios::beg);
        if (!file) { throw std::runtime_error("Could not seek file"); }

        std::vector<std::uint8_t> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) { throw std::runtime_error("Could not read file"); }
        return buffer;
    }

    std::optional<std::vector<std::uint8_t>>prefetch_guest_program(const GuestProgram& program, const GuestReg& EIP) 
    {
        const std::size_t offset = EIP.dword;
        if (offset >= program.size()) { return std::nullopt; }
        // An IA32 instruction is at most 15 bytes
        constexpr std::size_t max_prefetch_buffer_size = 15;
        const std::size_t remaining_guest_program_size = program.size() - offset;
        const std::size_t chunk_size = std::min(max_prefetch_buffer_size, remaining_guest_program_size);
        std::vector<std::uint8_t> prefetch_chunk(chunk_size);
        std::copy_n(program.begin() + offset, chunk_size, prefetch_chunk.begin());
        return prefetch_chunk;
    }
}

//============
// IR
//============
struct Value { std::uint32_t id; }; // Virtual register, not the host reg

struct Const32 { Value destination; std::uint32_t value; };
struct LoadReg32 { Value destination; ia32::GuestRegType reg; };
struct StoreReg32 { ia32::GuestRegType reg; Value source; };
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
// Translation Cache
//============

// a better version should have guest_pc + cpu context lookup
// chaining would require HostCodeInfo to have exit information
struct HostCodeInfo { std::uintptr_t start_addr; std::size_t length; }; // no chaining for now
std::unordered_map<std::uint32_t, HostCodeInfo> translation_cache;

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
    NotImplemented,
    EndOfGuestProgram,
    DecodeError
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

TranslationStatus dispatch(ia32::GuestProgram& guest_program, ia32::CPU& cpu_state, BlockFunction block_function)
{
    // lookup code in cache first, if not, itll will go through the decoder, translator
    // install code
    // dispatcher transfers control to the installed code
    const ia32::GuestReg& guest_eip = cpu_state.gen_purpose_registers[::EIP];
    if (!translation_cache.contains(guest_eip.dword))
    {

        ia32::InstructionBundle bundle = {};
        while(1)
        {
            auto result = ia32::prefetch_guest_program(guest_program, guest_eip);
            if (!result) { return TranslationStatus::EndOfGuestProgram; }

            ia32::Instruction* guest_instruction_ptr = ia32::decoder(result->data(), result->size());
            if (!guest_instruction_ptr) { return TranslationStatus::DecodeError; }
            ia32::Instruction guest_instruction = *guest_instruction_ptr;
            
            // decoder allocated instr with calloc
            std::free(guest_instruction_ptr);
            ia32::OpcodeID& id = guest_instruction->OpcodeID;

            if (!is_accepted(id)) { return TranslationStatus::InstructionNotAccepted; }

            if (id = ia32::OpcodeID::JMP_REL8  || 
                id = ia32::OpcodeID::JMP_REL32 || 
                id = ia32::OpcodeID::JE_REL8   ||
                id = ia32::OpcodeID::JNE_REL8  || )
            {
                bundle.push_back()
                break;
            }


        }

    }
    return TranslationStatus::NotImplemented;
}

int main(int argc, char *argv[])
{
    if (argc < 2) 
    {
        std::fprintf(stderr, "Usage: %s <guest-binary>\n", argv[0]);
        return 1;
    }

    asmjit::JitRuntime jit_runtime;
    asmjit::CodeHolder code;
    const asmjit::Error error = code.init(jit_runtime.environment());
    if (error != asmjit::Error::kOk) 
    {
        std::fprintf(stderr, "AsmJit initialization failed: %s\n", asmjit::stringify_error(error));
        return 1;
    }

    ia32::GuestProgram guest_program = ia32::read_binary_file(argv[1]);
    ia32::CPU guest_cpu_state{};

    return 0;
}
