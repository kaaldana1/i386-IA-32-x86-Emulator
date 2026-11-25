#include <asmjit/a64.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
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

    std::int32_t get_register_offset(GuestRegType reg)
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

namespace translator::direct_route 
{
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

namespace translator::ir 
{
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
    struct SetGuestEIP { Value value; };
    struct ExitBlock {};

    using operation = std::variant<Const32, LoadReg32, StoreReg32, Add32, IsZero32,
                                Equal32, LoadZF, StoreZF, Select32, SetGuestEIP,
                                ExitBlock>;
};

namespace aarch64 {
    using RegisterNumber = std::uint32_t;
    using BlockFunction = void (*)(ia32::CPU* cpu_state);

    class Emitter
    {
        public:
            explicit Emitter(asmjit::a64::Assembler& assembler, asmjit::CodeHolder& code, asmjit::JitRuntime& runtime)
                : assembler(assembler), code(code), runtime(runtime) {}

            asmjit::Error begin_block()
            {
                // Each block starts with an empty code buffer
                code.reset();
                const auto error = code.init(runtime.environment());
                if (error != asmjit::Error::kOk) { return error; }
                return code.attach(&assembler);
            }

            asmjit::Error load_immediate32(RegisterNumber destination, std::uint32_t immediate)
            {
                const auto low = immediate & 0xFFFFu;
                const auto high = immediate >> 16;
                const auto error = assembler.movz(asmjit::a64::w(destination), low, 0);
                if (error != asmjit::Error::kOk) { return error; }
                return assembler.movk(asmjit::a64::w(destination), high, 16);
            }

            // x0 holds the CPU pointer throughout the block, offset for the registers
            asmjit::Error load_cpu32(RegisterNumber destination, std::int32_t offset)
            {
                return assembler.ldr(asmjit::a64::w(destination), asmjit::a64::ptr(asmjit::a64::x0, offset));
            }

            asmjit::Error load_cpu16(RegisterNumber destination, std::int32_t offset)
            {
                return assembler.ldrh(asmjit::a64::w(destination), asmjit::a64::ptr(asmjit::a64::x0, offset));
            }

            asmjit::Error and_immediate32(RegisterNumber destination, RegisterNumber source, std::uint32_t mask)
            {
                return assembler.and_(asmjit::a64::w(destination), asmjit::a64::w(source), mask);
            }

            bool store_guest_zf(RegisterNumber result, std::int32_t status_reg_offset)
            {
                return (compare_immediate32(result, 0) == asmjit::Error::kOk &&
                       set_equal32(result) == asmjit::Error::kOk &&
                       write_guest_zf(result, status_reg_offset) == asmjit::Error::kOk);
            }

            // from source register to an address
            asmjit::Error store_cpu32(RegisterNumber source, std::int32_t offset)
            {
                return assembler.str(asmjit::a64::w(source), asmjit::a64::ptr(asmjit::a64::x0, offset));
            }

            asmjit::Error add32(RegisterNumber destination, RegisterNumber lhs, RegisterNumber rhs)
            {
                return assembler.add(asmjit::a64::w(destination), asmjit::a64::w(lhs), asmjit::a64::w(rhs));
            }

            asmjit::Error add_immediate32(RegisterNumber destination, RegisterNumber source, std::uint32_t immediate)
            {
                return assembler.add(asmjit::a64::w(destination), asmjit::a64::w(source), immediate);
            }

            asmjit::Error compare_immediate32(RegisterNumber source, std::uint32_t immediate)
            {
                return assembler.cmp(asmjit::a64::w(source), immediate);
            }

            asmjit::Error set_equal32(RegisterNumber destination)
            {
                // EQ tests the host Z flag set by a preceding comparison 
                return assembler.cset(asmjit::a64::w(destination), asmjit::arm::CondCode::kEQ);
            }

            asmjit::Error select_register(RegisterNumber destination, RegisterNumber if_true, RegisterNumber if_false, asmjit::arm::CondCode condition)
            {
                return assembler.csel(asmjit::a64::w(destination), asmjit::a64::w(if_true), asmjit::a64::w(if_false), condition);
            }

            asmjit::Error ret()
            {
                return assembler.ret(asmjit::a64::x30);
            }

            asmjit::Error code_install(BlockFunction& block_function)
            {
                return runtime.add(&block_function, &code);
            }

            asmjit::Error operation_to_host(translator::ir::operation operation)
            {
                // The frontend reuses IDs 0–3. Map them to w9–w12 and keep w13 for flag writes.
                constexpr auto status_reg_offset = static_cast<std::int32_t>(offsetof(ia32::CPU, status_register));
                const auto valid = [](translator::ir::Value value) { return value.id < 4; };
                const auto host_reg = [](translator::ir::Value value) { return 9u + value.id; };

                if (const auto* constant = std::get_if<translator::ir::Const32>(&operation))
                {
                    if (!valid(constant->destination)) { return asmjit::Error::kInvalidArgument; }
                    return load_immediate32(host_reg(constant->destination), constant->value);
                }

                if (const auto* load = std::get_if<translator::ir::LoadReg32>(&operation))
                {
                    if (!valid(load->destination)) { return asmjit::Error::kInvalidArgument; }
                    return load_cpu32(host_reg(load->destination), ia32::get_register_offset(load->reg));
                }

                if (const auto* store = std::get_if<translator::ir::StoreReg32>(&operation))
                {
                    if (!valid(store->source)) { return asmjit::Error::kInvalidArgument; }
                    return store_cpu32(host_reg(store->source), ia32::get_register_offset(store->reg));
                }

                if (const auto* add = std::get_if<translator::ir::Add32>(&operation))
                {
                    if (!valid(add->destination) || !valid(add->lhs) || !valid(add->rhs))
                    { return asmjit::Error::kInvalidArgument; }
                    return add32(host_reg(add->destination), host_reg(add->lhs), host_reg(add->rhs));
                }

                if (const auto* is_zero = std::get_if<translator::ir::IsZero32>(&operation))
                {
                    if (!valid(is_zero->destination) || !valid(is_zero->source)) { return asmjit::Error::kInvalidArgument; }
                    const auto error = compare_immediate32(host_reg(is_zero->source), 0);
                    if (error != asmjit::Error::kOk) { return error; }
                    return set_equal32(host_reg(is_zero->destination));
                }

                if (const auto* equal = std::get_if<translator::ir::Equal32>(&operation))
                {
                    if (!valid(equal->destination) || !valid(equal->lhs) || !valid(equal->rhs)) { return asmjit::Error::kInvalidArgument; }

                    const auto error = assembler.cmp(asmjit::a64::w(host_reg(equal->lhs)), asmjit::a64::w(host_reg(equal->rhs)));
                    if (error != asmjit::Error::kOk) { return error; }
                    return set_equal32(host_reg(equal->destination));
                }

                if (const auto* load = std::get_if<translator::ir::LoadZF>(&operation))
                {
                    if (!valid(load->destination)) { return asmjit::Error::kInvalidArgument; }

                    const auto destination = host_reg(load->destination);
                    const auto error = load_cpu16(destination, status_reg_offset);
                    if (error != asmjit::Error::kOk) { return error; }
                    return assembler.ubfx(asmjit::a64::w(destination), asmjit::a64::w(destination), 1, 1);
                }

                if (const auto* store = std::get_if<translator::ir::StoreZF>(&operation))
                {
                    if (!valid(store->source)) { return asmjit::Error::kInvalidArgument; }
                    return write_guest_zf(host_reg(store->source), status_reg_offset);
                }

                if (const auto* select = std::get_if<translator::ir::Select32>(&operation))
                {
                    if (!valid(select->destination) || !valid(select->condition) ||
                        !valid(select->if_true) || !valid(select->if_false))
                    { return asmjit::Error::kInvalidArgument; }
                    const auto error = compare_immediate32(host_reg(select->condition), 0);
                    if (error != asmjit::Error::kOk) { return error; }
                    return select_register(host_reg(select->destination), host_reg(select->if_true), host_reg(select->if_false), asmjit::arm::CondCode::kNE);
                }

                if (const auto* set_eip = std::get_if<translator::ir::SetGuestEIP>(&operation))
                {
                    if (!valid(set_eip->value)) { return asmjit::Error::kInvalidArgument; }
                    return store_cpu32(host_reg(set_eip->value), ia32::get_register_offset(ia32::GuestRegType::EIP));
                }
                if (std::holds_alternative<translator::ir::ExitBlock>(operation)) { return ret(); }
                return asmjit::Error::kInvalidArgument;
            }

        private:
            asmjit::Error write_guest_zf(RegisterNumber source, std::int32_t status_reg_offset)
            {
                constexpr RegisterNumber scratch = 13;
                const auto status = asmjit::a64::w(scratch);

                auto error = load_cpu16(scratch, status_reg_offset);
                if (error != asmjit::Error::kOk) { return error; }

                error = assembler.bic(status, status, ::ZF);
                if (error != asmjit::Error::kOk) { return error; }

                error = assembler.orr(status, status, asmjit::a64::w(source), asmjit::a64::lsl(1));
                if (error != asmjit::Error::kOk) { return error; }

                return assembler.strh(status, asmjit::a64::ptr(asmjit::a64::x0, status_reg_offset));
            }

            asmjit::a64::Assembler& assembler;
            asmjit::CodeHolder& code;
            asmjit::JitRuntime& runtime;
    };

}

namespace translator 
{
    enum class Status 
    { 
        NotImplemented = 1,
        EndOfGuestProgram,
        InstructionNotAccepted,
        Success,
        FailedDirectRoute,
        TranslationError,
        DecodeError,
        InvalidGuestPC,
        NoOperation,
        CodeInstallationError
    };

    // A better version should use guest_pc plus CPU context for lookup
    using GuestPC = std::uint32_t;
    using TranslationCache = std::unordered_map<GuestPC, aarch64::BlockFunction>;

};

namespace translator::ir 
{
    std::vector<operation> ignore_redundant_ZF_checks(std::vector<operation>& ir_operations);
    Status translate(const ia32::InstructionBundle& bundle, std::uint32_t guest_pc, aarch64::Emitter& emitter, bool optimize_ir)
    {
        std::vector<ir::operation> ir_operations;

        const auto store_result_and_zf = [&ir_operations](ia32::GuestRegType reg, ir::Value result) {
            ir_operations.push_back(ir::StoreReg32{.reg = reg, .source = result});
            ir_operations.push_back(ir::IsZero32{.destination = {3}, .source = result});
            ir_operations.push_back(ir::StoreZF{.source = {3}});
        };

        for (const ia32::Instruction& instruction : bundle)
        {
            const std::uint32_t next_pc = guest_pc + instruction.total_length;
            const auto guest_reg = static_cast<ia32::GuestRegType>(instruction.opcode[0] & 0x7);
            const auto rm_guest_reg = static_cast<ia32::GuestRegType>(instruction.rm_field);
            const auto modrm_reg = static_cast<ia32::GuestRegType>(instruction.reg_or_opcode);

            switch(instruction.OpcodeID)
            {
                case ia32::OpcodeID::MOV_ECX_IMM32: 
                case ia32::OpcodeID::MOV_EAX_IMM32: 
                case ia32::OpcodeID::MOV_EDX_IMM32:
                case ia32::OpcodeID::MOV_EBX_IMM32:
                {
                    const auto immediate = ia32::get_immediate32(instruction.immediate);
                    ir_operations.push_back(ir::LoadReg32{.destination = {0}, .reg = guest_reg});
                    ir_operations.push_back(ir::Const32{.destination = {1}, .value = immediate});
                    ir_operations.push_back(ir::StoreReg32{.reg = guest_reg, .source = {1}});
                    break;
                }

                case ia32::OpcodeID::INC_EAX:
                case ia32::OpcodeID::INC_ECX:
                case ia32::OpcodeID::INC_EDX:
                case ia32::OpcodeID::INC_EBX:
                {
                    ir_operations.push_back(ir::LoadReg32{.destination = {0}, .reg = guest_reg});
                    ir_operations.push_back(ir::Const32{.destination = {1}, .value = 1});
                    ir_operations.push_back(ir::Add32{.destination = {2}, .lhs = {0}, .rhs = {1}});
                    store_result_and_zf(guest_reg, ir::Value{2});
                    break;
                }

                case ia32::OpcodeID::JMP_REL8:
                case ia32::OpcodeID::JMP_REL32:
                {
                    const std::uint32_t displacement = (instruction.OpcodeID == ia32::OpcodeID::JMP_REL8) ?
                                                        static_cast<std::uint32_t>(static_cast<std::int8_t>(instruction.immediate[0])) : 
                                                        ia32::get_immediate32(instruction.immediate); 
                    const std::uint32_t target = next_pc + displacement;
                    ir_operations.push_back(ir::Const32{.destination = {0}, .value = target});
                    ir_operations.push_back(ir::SetGuestEIP{.value = {0}});
                    ir_operations.push_back(ir::ExitBlock{});
                    break;
                }

                case ia32::OpcodeID::JE_REL8:
                case ia32::OpcodeID::JNE_REL8:
                {
                    const std::uint32_t displacement = static_cast<std::uint32_t>(static_cast<std::int8_t>(instruction.immediate[0]));
                    const std::uint32_t target_address = next_pc + displacement;
                    const std::uint32_t fallback_address = next_pc;

                    ir_operations.push_back(ir::LoadZF{.destination = {0}});
                    ir_operations.push_back(ir::Const32{.destination = {1}, .value = target_address});
                    ir_operations.push_back(ir::Const32{.destination = {2}, .value = fallback_address});
                    const bool jump_if_zero = instruction.OpcodeID == ia32::OpcodeID::JE_REL8;
                    ir_operations.push_back(ir::Select32{.destination = {3}, .condition = {0}, .if_true = {jump_if_zero ? 1u : 2u}, .if_false = {jump_if_zero ? 2u : 1u}});
                    ir_operations.push_back(ir::SetGuestEIP{.value = {3}});
                    ir_operations.push_back(ir::ExitBlock{});
                    break;
                }

                case ia32::OpcodeID::ADD_R32_RM32:
                    // for now, translator only accepts register direct (no mem operands)
                    // bool is_register_direct = instruction.mod == 0x3; 
                    ir_operations.push_back(ir::LoadReg32{.destination = {0}, .reg = modrm_reg});
                    ir_operations.push_back(ir::LoadReg32{.destination = {1}, .reg = rm_guest_reg});
                    ir_operations.push_back(ir::Add32{.destination = {0}, .lhs = {0}, .rhs = {1}});
                    store_result_and_zf(modrm_reg, ir::Value{0});
                    break;

                case ia32::OpcodeID::IMM_GRP_EV_LB:
                {
                    uint32_t immediate = (uint32_t)(int32_t)(int8_t)instruction.immediate[0];
                    ir_operations.push_back(ir::LoadReg32{.destination = {0}, .reg = rm_guest_reg});
                    ir_operations.push_back(ir::Const32{.destination = {1}, .value = immediate});
                    ir_operations.push_back(ir::Add32{.destination = {2}, .lhs = {0}, .rhs = {1}});
                    store_result_and_zf(rm_guest_reg, ir::Value{2});
                    break;
                }

                case ia32::OpcodeID::CMP_R32_RM32:
                    ir_operations.push_back(ir::LoadReg32{.destination = {0}, .reg = modrm_reg});
                    ir_operations.push_back(ir::LoadReg32{.destination = {1}, .reg = rm_guest_reg});
                    ir_operations.push_back(ir::Equal32{.destination = {2}, .lhs = {0}, .rhs = {1}});
                    ir_operations.push_back(ir::StoreZF{.source = {2}});
                    break;

                default:
                    return Status::NotImplemented;
            }
            guest_pc = next_pc;
        }
        // frontend done, now time to emit
        if (ir_operations.empty()) { return Status::NoOperation; }
        if (!std::holds_alternative<ir::ExitBlock>(ir_operations.back()))
        {
            ir_operations.push_back(ir::Const32{.destination = {0}, .value = guest_pc});
            ir_operations.push_back(ir::SetGuestEIP{.value = {0}});
            ir_operations.push_back(ir::ExitBlock{});
        }

        if (optimize_ir) 
        { 
            auto optimized_ir = ignore_redundant_ZF_checks(ir_operations); 
            ir_operations.swap(optimized_ir);
        }

        for (const auto& operation : ir_operations)
        {
            if (emitter.operation_to_host(operation) != asmjit::Error::kOk)
            { return Status::TranslationError; }
        }
        return Status::Success;
    }

    std::vector<operation> ignore_redundant_ZF_checks(std::vector<operation>& ir_operations)
    {
        bool ZF_needed = true;
        std::vector<operation> optimized_ir_block = {};
        for (std::size_t i = ir_operations.size(); i > 0; i--)
        {
            const auto operation = ir_operations[i-1];
            if (const auto *load_ZF = std::get_if<translator::ir::LoadZF>(&operation)) { ZF_needed = true; }
            else if (const auto *store_ZF = std::get_if<translator::ir::StoreZF>(&operation))
            {
                if (!ZF_needed) 
                {
                        const ir::operation& previous = ir_operations[i - 2];
                        const ir::IsZero32* test = std::get_if<ir::IsZero32>(&previous);
                        const ir::Equal32* equal = std::get_if<ir::Equal32>(&previous);
                        if ((test && test->destination.id == store_ZF->source.id) 
                            || (equal && equal->destination.id == store_ZF->source.id))
                        { --i; }
                    continue; // skip both the storeZF and the test
                }
                // if there are any more store & test ZF behind this one that is not followed by their own load ZF, then 
                // the results of those tests are not read and can be ignored, keeping ZF_needed false
                ZF_needed = false; 
            } 
            optimized_ir_block.push_back(operation);
        }

        std::reverse(optimized_ir_block.begin(), optimized_ir_block.end());
        return optimized_ir_block;
    }

}
namespace translator::direct_route
{
    Status translate(const ia32::InstructionBundle& bundle, std::uint32_t guest_pc, aarch64::Emitter& emitter)
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
                        emitter.store_cpu32(host_scratch_register, ia32::get_register_offset(guest_reg)) != Error::kOk)
                    { return Status::FailedDirectRoute; }
                    break;
                }
                case ia32::OpcodeID::INC_EAX:
                case ia32::OpcodeID::INC_ECX:
                case ia32::OpcodeID::INC_EDX:
                case ia32::OpcodeID::INC_EBX:
                {
                    const auto cpu_offset = ia32::get_register_offset(guest_reg);
                    const auto status_reg_offset = static_cast<std::int32_t>(offsetof(ia32::CPU, status_register));
                    if (emitter.load_cpu32(host_scratch_register, cpu_offset) != Error::kOk ||
                        emitter.add_immediate32(host_scratch_register, host_scratch_register, 1) != Error::kOk ||
                        emitter.store_cpu32(host_scratch_register, cpu_offset) != Error::kOk || 
                        emitter.store_guest_zf(host_scratch_register, status_reg_offset) == false )
                    { return Status::FailedDirectRoute; }
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
                        emitter.store_cpu32(host_scratch_register, ia32::get_register_offset(ia32::GuestRegType::EIP)) != Error::kOk ||
                        emitter.ret() != Error::kOk)
                    { return Status::FailedDirectRoute; }
                    return Status::Success;
                }
                case ia32::OpcodeID::JE_REL8:
                case ia32::OpcodeID::JNE_REL8:
                {
                    // Jump to target if ZF = 1
                    // Host side:
                    // scratch reg = cpu.status_reg & ZF
                    // fallback address = eip + instruction.length
                    // calculate target same way jmp_rel
                    // mov target and fallback_address to host reg1 and reg2 respectively
                    // CMP scratc_reg, 0 -> sets host's Z flag 
                    // CSEL(dest_reg, reg1, reg2, Z)
                    // str dest_reg into EIP and ret

                    std::int32_t status_reg_offset = static_cast<std::int32_t>(offsetof(ia32::CPU, status_register));
                    std::uint32_t fallback_address = next_pc;
                    std::uint32_t displacement = static_cast<std::uint32_t>(static_cast<std::int8_t>(instruction.immediate[0]));
                    std::uint32_t target_address = next_pc + displacement;
                    aarch64::RegisterNumber host_target_reg = 10;
                    aarch64::RegisterNumber host_fallback_reg = 11;
                    aarch64::RegisterNumber host_result_reg = 12;
                    const asmjit::arm::CondCode condition = (instruction.OpcodeID == ia32::OpcodeID::JE_REL8) ?
                                                                asmjit::arm::CondCode::kNE : asmjit::arm::CondCode::kEQ;
                    if (emitter.load_immediate32(host_target_reg, target_address) != Error::kOk ||
                        emitter.load_immediate32(host_fallback_reg, fallback_address) != Error::kOk ||
                        emitter.load_cpu16(host_scratch_register, status_reg_offset) != Error::kOk ||
                        emitter.and_immediate32(host_scratch_register, host_scratch_register, ::ZF) != Error::kOk ||
                        emitter.compare_immediate32(host_scratch_register, 0) != Error::kOk ||
                        emitter.select_register(host_result_reg, host_target_reg, host_fallback_reg, condition) != Error::kOk ||
                        emitter.store_cpu32(host_result_reg, ia32::get_register_offset(ia32::GuestRegType::EIP)) != Error::kOk ||
                        emitter.ret() != Error::kOk)
                    { return Status::FailedDirectRoute; }
                    return Status::Success;
                }
                default:
                    return Status::FailedDirectRoute;
            }
            guest_pc = next_pc;
        }
        if (emitter.load_immediate32(host_scratch_register, guest_pc) != Error::kOk ||
            emitter.store_cpu32(host_scratch_register, ia32::get_register_offset(ia32::GuestRegType::EIP)) != Error::kOk ||
            emitter.ret() != Error::kOk)
        { return Status::FailedDirectRoute; }
        return Status::Success;
    }
}

namespace translator
{
    Status route_translation(const ia32::InstructionBundle& bundle, std::uint32_t guest_pc, aarch64::Emitter& emitter, 
                             bool force_ir, bool optimize_ir)
    {
        if (force_ir) { return ir::translate(bundle, guest_pc, emitter, optimize_ir); }
        // whole bundle needs to be direct route or itll go to IR translation
        for (const ia32::Instruction& instruction : bundle)
        {
            const ia32::OpcodeID& id = instruction.OpcodeID;
            if (!direct_route::is_direct_eligible(id))
            {
                return ir::translate(bundle, guest_pc, emitter, optimize_ir=false);
            }
        }
        return direct_route::translate(bundle, guest_pc, emitter);
    }

    Status dispatch(const ia32::GuestProgram& program, ia32::CPU& cpu, aarch64::Emitter& emitter, TranslationCache& translation_cache, 
                    bool force_ir, bool optimize_ir)
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
            if (emitter.begin_block() != asmjit::Error::kOk) { return Status::TranslationError; }
            if (route_translation(bundle, guest_eip, emitter, force_ir, optimize_ir) != Status::Success) { return Status::TranslationError; }

            aarch64::BlockFunction block_function = nullptr;
            if (emitter.code_install(block_function) != asmjit::Error::kOk) { return Status::CodeInstallationError; }
            translation_cache[guest_eip] = block_function;

        }
        translation_cache[guest_eip](&cpu);
        return Status::Success;
    }
}

int main(int argc, char *argv[])
{
    bool force_ir = false;
    bool optimize_ir  = false;
    const char* guest_binary = nullptr;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--force_ir") == 0) { force_ir = true; }
        else if (std::strcmp(argv[i], "--optimize_ir") == 0) { force_ir = true; optimize_ir = true; }
        else if (!guest_binary) { guest_binary = argv[i]; }
        else { guest_binary = nullptr; break; }
    }
    if (!guest_binary)
    {
        std::fprintf(stderr, "Usage: %s [--force_ir] or [--optimize_ir] <guest-binary>\n", argv[0]);
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
    aarch64::Emitter emitter(assembler, code, jit_runtime);

    ia32::GuestProgram guest_program = ia32::read_binary_file(guest_binary);
    ia32::CPU guest_cpu_state{};

    translator::TranslationCache translation_cache;
    translator::Status result = translator::Status::NotImplemented;
    do {
        result = translator::dispatch(guest_program, guest_cpu_state, emitter, translation_cache, force_ir, optimize_ir);
    }
    while (result == translator::Status::Success);
    std::printf("EAX=%u ECX=%u EIP=%u ZF=%d\n", guest_cpu_state.gen_purpose_registers[::EAX].dword, guest_cpu_state.gen_purpose_registers[::ECX].dword, guest_cpu_state.gen_purpose_registers[::EIP].dword, (guest_cpu_state.status_register >> 1) & 1);
    if (result == translator::Status::EndOfGuestProgram) { return 0; }
    return static_cast<int>(result);
}
