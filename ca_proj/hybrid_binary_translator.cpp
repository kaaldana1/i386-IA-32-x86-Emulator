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

    using ::instr_metadata_lut;
    using ::single_byte_opcode_lut;
    using ::decoder;

    using InstructionBundle = std::vector<Instruction>;
    using GuestProgram = std::vector<std::uint8_t>;

    bool is_accepted(const Instruction& instruction)
    {
        if (instruction.prefix_length != 0 || instruction.opcode_length != 1) { return false; }

        switch (instruction.OpcodeID)
        {
            case OpcodeID::MOV_EAX_IMM32:
            case OpcodeID::MOV_ECX_IMM32:
            case OpcodeID::MOV_EDX_IMM32:
            case OpcodeID::MOV_EBX_IMM32:
            case OpcodeID::INC_EAX:
            case OpcodeID::INC_ECX:
            case OpcodeID::INC_EDX:
            case OpcodeID::INC_EBX:
            case OpcodeID::JMP_REL8:
            case OpcodeID::JMP_REL32:
            case OpcodeID::JE_REL8:
            case OpcodeID::JNE_REL8:
                return true;
            case OpcodeID::ADD_R32_RM32:
            case OpcodeID::CMP_R32_RM32:
                return instruction.mod == 3 && instruction.reg_or_opcode < 4 && instruction.rm_field < 4;
            case OpcodeID::IMM_GRP_EV_LB:
                return instruction.mod == 3 && instruction.reg_or_opcode == 0 && instruction.rm_field < 4;
            default:
                return false;
        }
    }

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

    std::optional<std::vector<std::uint8_t>> prefetch_guest_program(const GuestProgram& program, std::size_t offset)
    {
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

namespace direct_route {
    // current support has more direct elibible instruction since im using simpler instructions
    // that may easily map from ia32 to aarch64,
    // but a full implementation would probably need to use IR more -> longer list
    
    // A linear scan is enough for the small opcode set and short blocks.
    constexpr ia32::OpcodeID direct_eligible[] = {
        ia32::OpcodeID::MOV_EAX_IMM32, ia32::OpcodeID::MOV_ECX_IMM32,
        ia32::OpcodeID::MOV_EDX_IMM32, ia32::OpcodeID::MOV_EBX_IMM32,
        ia32::OpcodeID::INC_EAX, ia32::OpcodeID::INC_ECX,
        ia32::OpcodeID::INC_EDX, ia32::OpcodeID::INC_EBX,
        ia32::OpcodeID::JMP_REL8, ia32::OpcodeID::JMP_REL32, 
        ia32::OpcodeID::JE_REL8, ia32::OpcodeID::JNE_REL8
    };

    bool is_direct_eligible(ia32::OpcodeID opcode)
    {
        for (const ia32::OpcodeID eligible : direct_eligible) {
            if (opcode == eligible) return true;
        }
        return false;
    }

};

namespace ir {
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
    struct SetGuestEIP { std::variant<Value, std::uint32_t> value; };
    struct ExitBlock {};

    using IR_object = std::variant<Const32, LoadReg32, StoreReg32, Add32, IsZero32,
                                Equal32, LoadZF, StoreZF, Select32, SetGuestEIP,
                                ExitBlock>;

    using Block = std::vector<IR_object>;
};

namespace jit { class ARM64Emitter; }

namespace translator {
    enum class Status 
    { 
        NotImplemented = 1,
        EndOfGuestProgram,
        InstructionNotAccepted,
        Success,
        FailedDirectRoute,
        DecodeError,
        InvalidGuestPC
    };

    class Translator {
        public:
            Status dispatch(const ia32::GuestProgram& program, ia32::CPU& cpu, jit::ARM64Emitter& emitter);

        private:
            Status translate(const ia32::InstructionBundle& bundle, jit::ARM64Emitter& emitter);
            // install(), cache lookup, etc.

            // a better version should have guest_pc + cpu context lookup
            // chaining would require HostCodeInfo to have exit information
            struct HostCodeInfo { std::uintptr_t start_addr; std::size_t length; }; // no chaining for now
            std::unordered_map<std::uint32_t, HostCodeInfo> translation_cache;
    };

    

};


namespace aarch64 
{
    using HostRegisterNumber = std::uint32_t;

    enum class Mnemonic 
    {
        MOVZ, MOVK, 
        LDR, 
        STR, 
        ADD_IMMEDIATE, ADD, 
        CMP, 
        CSET, 
        RET
    };

}



namespace jit {
    using BlockFunction = void (*)(ia32::CPU* cpu_state);

    class ARM64Emitter {
        public:
            explicit ARM64Emitter(asmjit::a64::Assembler& assembler) : assembler(assembler) {}

            asmjit::Error emit(aarch64::Mnemonic mnemonic, aarch64::HostRegisterNumber destination,
                aarch64::HostRegisterNumber source, std::uint64_t immediate = 0,
                std::int32_t cpu_offset = 0, std::uint32_t shift = 0)
                {
                    namespace a64 = asmjit::a64;

                    switch (mnemonic) {
                        // MOVZ/MOVK take a 16-bit immediate and a shift of 0 or 16 for W regs
                        case aarch64::Mnemonic::MOVZ:
                            return assembler.movz(a64::w(destination), immediate, shift);
                        case aarch64::Mnemonic::MOVK:
                            return assembler.movk(a64::w(destination), immediate, shift);
                        // x0 holds the CPU pointer throughout the block. Use other registers for scratch.
                        case aarch64::Mnemonic::LDR:
                            return assembler.ldr(a64::w(destination), a64::ptr(a64::x0, cpu_offset));
                        case aarch64::Mnemonic::STR:
                            return assembler.str(a64::w(destination), a64::ptr(a64::x0, cpu_offset));
                        case aarch64::Mnemonic::ADD_IMMEDIATE:
                            return assembler.add(a64::w(destination), a64::w(source), immediate);
                        case aarch64::Mnemonic::ADD:
                            return assembler.add(a64::w(destination), a64::w(destination), a64::w(source));
                        case aarch64::Mnemonic::CMP:
                            return assembler.cmp(a64::w(destination), immediate);
                        case aarch64::Mnemonic::CSET:
                            return assembler.cset(a64::w(destination), asmjit::arm::CondCode::kEQ);
                        case aarch64::Mnemonic::RET:
                            return assembler.ret(a64::x30);
                    }
                    return asmjit::Error::kInvalidInstruction;
                }

        private:
            asmjit::a64::Assembler& assembler;
    };

}

namespace jit 
{
    translator::Status code_install(asmjit::JitRuntime&, asmjit::CodeHolder&, BlockFunction&)
    {
        
        return translator::Status::NotImplemented;
    }
}


namespace ir 
{
    translator::Status translate(const ia32::InstructionBundle&, jit::ARM64Emitter&)
    {
        // This is a placeholder for the IR translation logic.
        // Lower the whole bundle to IR, then emit AArch64.
        return translator::Status::NotImplemented;
    }
}
namespace direct_route
{
    translator::Status translate(const ia32::InstructionBundle& bundle, jit::ARM64Emitter& emitter)
    {
        using aarch64::Mnemonic;
        using asmjit::Error;

        // instead of mapping each guest reg to host reg
        // reusing the same host reg is safe
        // because each host instruction updates the guest state CPU, which have their own unique slots 
            // (ie eax or ecx gets updated)
        // when host machine moves onto the next instruction, it can reuse the same host reg
        aarch64::HostRegisterNumber a64_scratch = 9;

        for (const ia32::Instruction& instruction : bundle)
        {
            // MOV and INC encode their register in the low three bits of the opcode.
            const auto guest_reg = instruction.opcode[0] & 0x7;
            const auto cpu_offset = static_cast<std::int32_t>(
                offsetof(ia32::CPU, gen_purpose_registers) + guest_reg * sizeof(ia32::GuestReg));

            switch (instruction.OpcodeID)
            {
                case ia32::OpcodeID::MOV_ECX_IMM32: 
                case ia32::OpcodeID::MOV_EAX_IMM32: 
                case ia32::OpcodeID::MOV_EDX_IMM32:
                case ia32::OpcodeID::MOV_EBX_IMM32:
                {
                    const auto low = std::uint32_t(instruction.immediate[0]) | (std::uint32_t(instruction.immediate[1]) << 8);
                    const auto high = std::uint32_t(instruction.immediate[2]) | (std::uint32_t(instruction.immediate[3]) << 8);
                    if (emitter.emit(Mnemonic::MOVZ, a64_scratch, 0, low) != Error::kOk ||
                        emitter.emit(Mnemonic::MOVK, a64_scratch, 0, high, 0, 16) != Error::kOk ||
                        emitter.emit(Mnemonic::STR, a64_scratch, 0, 0, cpu_offset) != Error::kOk )
                    {
                        return translator::Status::FailedDirectRoute;
                    }
                    break;
                }
                case ia32::OpcodeID::INC_EAX:
                case ia32::OpcodeID::INC_ECX:
                case ia32::OpcodeID::INC_EDX:
                case ia32::OpcodeID::INC_EBX:
                {
                    if (emitter.emit(Mnemonic::LDR, a64_scratch, 0, 0, cpu_offset) != Error::kOk ||
                        emitter.emit(Mnemonic::ADD_IMMEDIATE, a64_scratch, a64_scratch, 1) != Error::kOk ||
                        emitter.emit(Mnemonic::STR, a64_scratch, 0, 0, cpu_offset) != Error::kOk )
                    {
                        return translator::Status::FailedDirectRoute;
                    }
                    // TODO: update guest ZF from the result.
                    break;
                }

                case ia32::OpcodeID::JMP_REL8:
                case ia32::OpcodeID::JMP_REL32:
                case ia32::OpcodeID::JE_REL8:
                case ia32::OpcodeID::JNE_REL8:
                    return translator::Status::NotImplemented;
                default:
                    return translator::Status::FailedDirectRoute;
            }
        }
        // TODO: emit the fall-through guest EIP and return to the dispatcher.
        return translator::Status::NotImplemented;
    }
}

translator::Status translator::Translator::translate(const ia32::InstructionBundle& bundle, jit::ARM64Emitter& emitter)
{
    // whole bundle needs to be direct route or itll go to IR translation
    for (const ia32::Instruction& instruction : bundle)
    {
        const ia32::OpcodeID& id = instruction.OpcodeID;
        if (!direct_route::is_direct_eligible(id))
        {
            return ir::translate(bundle, emitter);
        }
    }
    return direct_route::translate(bundle, emitter);
}

translator::Status translator::Translator::dispatch(const ia32::GuestProgram& program, ia32::CPU& cpu, jit::ARM64Emitter& emitter)
{
    // lookup code in cache first, if not, itll will go through the decoder, translator
    // install code
    // dispatcher transfers control to the installed code
    const auto guest_eip = cpu.gen_purpose_registers[::EIP].dword;
    if (guest_eip == program.size()) { return Status::EndOfGuestProgram; }
    if (guest_eip > program.size()) { return Status::InvalidGuestPC; }

    if (!translation_cache.contains(guest_eip))
    {
        // the decoder phase needs a copy of the EIP to inch forward, rather than use the EIP register,
        // CPU state is changed by the host code
        constexpr std::size_t max_block_instructions = 8;
        std::size_t decode_pc = guest_eip;
        ia32::InstructionBundle bundle;
        bundle.reserve(max_block_instructions);
        while (bundle.size() < max_block_instructions && decode_pc < program.size())
        {
            auto guest_bytes = ia32::prefetch_guest_program(program, decode_pc);
            if (!guest_bytes) { return Status::DecodeError; }

            ia32::Instruction* guest_instruction_ptr = ia32::decoder(guest_bytes->data(), guest_bytes->size());
            if (!guest_instruction_ptr) { return Status::DecodeError; }

            ia32::Instruction guest_instruction = *guest_instruction_ptr;
            std::free(guest_instruction_ptr);
            if (!ia32::is_accepted(guest_instruction)) { return Status::InstructionNotAccepted; }

            bundle.push_back(guest_instruction);
            decode_pc += guest_instruction.total_length;

            const auto id = guest_instruction.OpcodeID;
            if (id == ia32::OpcodeID::JMP_REL8  || id == ia32::OpcodeID::JMP_REL32 || 
                id == ia32::OpcodeID::JE_REL8   || id == ia32::OpcodeID::JNE_REL8 ) { break; }
        }
        return translate(bundle, emitter);
    }
    return Status::NotImplemented;
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

    asmjit::a64::Assembler assembler(&code);
    jit::ARM64Emitter emitter(assembler);

    ia32::GuestProgram guest_program = ia32::read_binary_file(argv[1]);
    ia32::CPU guest_cpu_state{};
    translator::Translator translator_instance;
    translator::Status result = translator::Status::NotImplemented;
    do {
        result = translator_instance.dispatch(guest_program, guest_cpu_state, emitter);
    }
    while (result == translator::Status::Success);
    if (result == translator::Status::EndOfGuestProgram) { return 0; }
    return static_cast<int>(result);
}
