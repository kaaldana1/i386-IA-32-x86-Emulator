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

    std::int32_t register_offset(GuestRegType reg)
    {
        return static_cast<std::int32_t>(offsetof(CPU, gen_purpose_registers) + reg * sizeof(GuestReg));
    }

    using Instruction = ::Instruction;
    using OpcodeID = ::OpcodeID;
    using InstructionMetadata = ::InstructionMetadata;

    using ::instr_metadata_lut;
    using ::single_byte_opcode_lut;
    using ::decoder;

    std::uint32_t get_immediate32(const std::uint8_t (&immediate)[4])
    {
        return std::uint32_t{immediate[0]} |
               (std::uint32_t{immediate[1]} << 8) |
               (std::uint32_t{immediate[2]} << 16) |
               (std::uint32_t{immediate[3]} << 24);
    }
    // acceptance depnds not only on the opcode but the operands
    // example would be ADD_R32_RM32 (either reg-reg or reg-mem) but we only accept reg-reg
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


    using InstructionBundle = std::vector<Instruction>;
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
            Status translate(const ia32::InstructionBundle& bundle, std::uint32_t guest_pc, jit::ARM64Emitter& emitter);
            // install(), cache lookup, etc.

            // a better version should have guest_pc + cpu context lookup
            // chaining would require HostCodeInfo to have exit information
            struct HostCodeInfo { std::uintptr_t start_addr; std::size_t length; }; // no chaining for now
            std::unordered_map<std::uint32_t, HostCodeInfo> translation_cache;
    };

    

};


namespace aarch64 
{
    using RegisterNumber = std::uint32_t;
}



namespace jit {
    using BlockFunction = void (*)(ia32::CPU* cpu_state);

    class ARM64Emitter
    {
        public:
            explicit ARM64Emitter(asmjit::a64::Assembler& assembler)
                : assembler(assembler) {}

            asmjit::Error load_immediate32(aarch64::RegisterNumber destination, std::uint32_t immediate)
            {
                const auto low = immediate & 0xFFFFu;
                const auto high = immediate >> 16;
                const auto error = assembler.movz(asmjit::a64::w(destination), low, 0);
                if (error != asmjit::Error::kOk) { return error; }
                return assembler.movk(asmjit::a64::w(destination), high, 16);
            }

            // x0 holds the CPU pointer throughout the block, offset for the registers
            asmjit::Error load_cpu32(aarch64::RegisterNumber destination, std::int32_t offset)
            {
                return assembler.ldr(asmjit::a64::w(destination), asmjit::a64::ptr(asmjit::a64::x0, offset));
            }

            asmjit::Error store_cpu32(aarch64::RegisterNumber source, std::int32_t offset)
            {
                return assembler.str(asmjit::a64::w(source), asmjit::a64::ptr(asmjit::a64::x0, offset));
            }

            asmjit::Error add32(aarch64::RegisterNumber destination, aarch64::RegisterNumber lhs, aarch64::RegisterNumber rhs)
            {
                return assembler.add(asmjit::a64::w(destination), asmjit::a64::w(lhs), asmjit::a64::w(rhs));
            }

            asmjit::Error add_immediate32(aarch64::RegisterNumber destination, aarch64::RegisterNumber source, std::uint32_t immediate)
            {
                return assembler.add(asmjit::a64::w(destination), asmjit::a64::w(source), immediate);
            }

            asmjit::Error compare_immediate32(aarch64::RegisterNumber source, std::uint32_t immediate)
            {
                return assembler.cmp(asmjit::a64::w(source), immediate);
            }

            asmjit::Error set_equal32(aarch64::RegisterNumber destination)
            {
                // EQ tests the host Z flag set by a preceding comparison.
                return assembler.cset(asmjit::a64::w(destination), asmjit::arm::CondCode::kEQ);
            }

            asmjit::Error ret()
            {
                return assembler.ret(asmjit::a64::x30);
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
    translator::Status translate(const ia32::InstructionBundle&, std::uint32_t, jit::ARM64Emitter&)
    {
        // This is a placeholder for the IR translation logic.
        // Lower the whole bundle to IR, then emit AArch64.
        return translator::Status::NotImplemented;
    }
}
namespace direct_route
{
    translator::Status translate(const ia32::InstructionBundle& bundle, std::uint32_t guest_pc, jit::ARM64Emitter& emitter)
    {
        using asmjit::Error;

        // instead of mapping each guest reg to host reg
        // reusing the same host reg is safe
        // because each guest instruction stores its result in its own CPU slot
        // before the next guest instruction reuses the scratch register
        constexpr aarch64::RegisterNumber host_scratch_register = 9;

        for (const ia32::Instruction& instruction : bundle)
        {
            const std::uint32_t next_pc = guest_pc + instruction.total_length;
            // MOV and INC encode their register in the low three bits of the opcode
            // ldr dest, [x0, cpu_offset], where x0 holds cpu*
            const auto guest_reg = static_cast<ia32::GuestRegType>(instruction.opcode[0] & 0x7);

            switch (instruction.OpcodeID)
            {
                case ia32::OpcodeID::MOV_ECX_IMM32: 
                case ia32::OpcodeID::MOV_EAX_IMM32: 
                case ia32::OpcodeID::MOV_EDX_IMM32:
                case ia32::OpcodeID::MOV_EBX_IMM32:
                {
                    const auto immediate = ia32::get_immediate32(instruction.immediate);

                    if (emitter.load_immediate32(host_scratch_register, immediate) != Error::kOk || 
                        emitter.store_cpu32(host_scratch_register, ia32::register_offset(guest_reg)) != Error::kOk)
                    { return translator::Status::FailedDirectRoute; }
                    break;
                }
                case ia32::OpcodeID::INC_EAX:
                case ia32::OpcodeID::INC_ECX:
                case ia32::OpcodeID::INC_EDX:
                case ia32::OpcodeID::INC_EBX:
                {
                    const auto cpu_offset = ia32::register_offset(guest_reg);
                    if (emitter.load_cpu32(host_scratch_register, cpu_offset) != Error::kOk ||
                        emitter.add_immediate32(host_scratch_register, host_scratch_register, 1) != Error::kOk ||
                        emitter.store_cpu32(host_scratch_register, cpu_offset) != Error::kOk)
                    { return translator::Status::FailedDirectRoute; }
                    // TODO: update guest ZF from the result
                    break;
                }

                case ia32::OpcodeID::JMP_REL8:
                case ia32::OpcodeID::JMP_REL32:
                {
                    const std::uint32_t displacement = (instruction.OpcodeID == ia32::OpcodeID::JMP_REL8) ?
                                                        static_cast<std::uint32_t>(static_cast<std::int8_t>(instruction.immediate[0])) : 
                                                        ia32::get_immediate32(instruction.immediate);
                    const std::uint32_t target = next_pc + displacement;
                    if (emitter.load_immediate32(host_scratch_register, target) != Error::kOk ||
                        emitter.store_cpu32(host_scratch_register, ia32::register_offset(ia32::GuestRegType::EIP)) != Error::kOk ||
                        emitter.ret() != Error::kOk)
                    { return translator::Status::FailedDirectRoute; }

                    // Installing and executing the emitted block is still pending
                    return translator::Status::NotImplemented;
                }
                case ia32::OpcodeID::JE_REL8:
                // Jump to target if ZF = 1
                // Host side:
                // Find offset of ZF, load into scratch reg
                // load guest EIP into host target reg
                // CMP scratc_reg, 0 -> sets host's N(negative) flag if ZF = 0, clears N if ZF = 1
                // CSET , kEQ -> 
                case ia32::OpcodeID::JNE_REL8:
                    // TODO: read guest ZF and select the target or fall-through EIP
                    return translator::Status::NotImplemented;
                default:
                    return translator::Status::FailedDirectRoute;
            }
            guest_pc = next_pc;
        }
        return translator::Status::NotImplemented;
    }
}

translator::Status translator::Translator::translate(const ia32::InstructionBundle& bundle, std::uint32_t guest_pc, jit::ARM64Emitter& emitter)
{
    // whole bundle needs to be direct route or itll go to IR translation
    for (const ia32::Instruction& instruction : bundle)
    {
        const ia32::OpcodeID& id = instruction.OpcodeID;
        if (!direct_route::is_direct_eligible(id))
        {
            return ir::translate(bundle, guest_pc, emitter);
        }
    }
    return direct_route::translate(bundle, guest_pc, emitter);
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
        return translate(bundle, guest_eip, emitter);
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
