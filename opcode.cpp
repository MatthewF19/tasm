#include <cstdint>
#include <cstring>
#include <optional>
#include <variant>
#include <vector>

namespace opcodes {
    const auto prefix_64bit = std::byte(0b0100'1000);
    const auto prefix_16bit = std::byte(0x66);

    enum class register_32 {
        EAX,
        ECX,
        EDX,
        EBX,
        ESP,
        EBP,
        ESI,
        EDI,
        R8D,
        R9D,
        R10D,
        R11D,
        R12D,
        R13D,
        R14D,
        R15D,
    };

    // TODO add 16 and 64 bit registers
    using reg = std::variant<register_32>;
    using imm = std::variant<int32_t, int16_t, int64_t>;

    using operand = std::variant<std::monostate, reg, imm>;

    std::byte operandValueAsRegister(const operand& op) {
        return static_cast<std::byte>(std::get<register_32>(std::get<reg>(op)));
    }

    int operandValueAsImmediate(const operand& op) {
        return static_cast<int>(std::get<int32_t>(std::get<imm>(op)));
    }

    enum class operation {
        MOV,
        ADD,
        SUB,
        SYSCALL,
        JMP,
    };

    enum class bitsize {
        b32,
        b16,
        b64,
    };

    std::optional<std::byte> bitsizePrefix(const bitsize& bitsize) {
        if(bitsize == bitsize::b16) { return prefix_16bit; } 
        else if(bitsize == bitsize::b64) { return prefix_64bit; } 
        else { return std::nullopt; }
    }

    // calculate the modrm byte give if it's direct register mode, the reg field, and the rm field
    std::byte modrm(bool registerDirect = true, 
                    std::byte reg = std::byte(0), 
                    std::byte rm = std::byte(0)) {
        std::byte modrm{};

        // mod field
        if(registerDirect) { modrm |= static_cast<std::byte>(0b11000000); }
        // reg field
        modrm |= reg;
        // rm field
        modrm |= rm;

        return modrm;
    }

    // convert a number to LE byte array (for immediates)
    template<typename T>
    std::vector<std::byte> toBytesLE(T val) {
        std::vector<std::byte> bytes(sizeof(T));
        std::memcpy(bytes.data(), &val, sizeof(T));

        return bytes;
    }

    // encode an operation with its operands
    std::vector<std::byte> encode(const operation& opn, 
                                  const operand& op1 = std::monostate{},
                                  const operand& op2 = std::monostate{},
                                  const operand& op3 = std::monostate{},
                                  const bitsize& bitsize = bitsize::b32) {
        std::vector<std::byte> opcode;

        // add bitsize prefix if needed
        auto prefix = bitsizePrefix(bitsize);
        if(prefix) { opcode.push_back(prefix.value()); }

        switch (opn) {
            case operation::MOV: 
              {
                // mov reg, imm
                if(std::holds_alternative<imm>(op2)) {
                    // primary opcode
                    // TODO get<register_32> fails with other sizes
                    auto primary = std::byte(0xB8 + static_cast<int>(std::get<register_32>(std::get<reg>(op1))));
                    opcode.push_back(primary); 
                    // immediate
                    // TODO get<int32_32> fails with other sizes
                    std::vector<std::byte> immediate = toBytesLE(std::get<int32_t>(std::get<imm>(op2)));
                    opcode.insert(opcode.end(), immediate.begin(), immediate.end());
                // mov reg, reg
                } else {
                    auto primary = std::byte(0x89);
                    std::byte mod = modrm(true, operandValueAsRegister(op2) << 3, operandValueAsRegister(op1)); 
                    opcode.push_back(primary);
                    opcode.push_back(mod);
                }
                break;
              } 
            case operation::ADD:
              {
                // add reg, imm
                if(std::holds_alternative<imm>(op2)) {
                    // primary opcode
                    auto primary = std::byte(0x81);
                    opcode.push_back(primary); 
                    // modrm
                    std::byte reg{0};
                    std::byte rm = operandValueAsRegister(op1);
                    opcode.emplace_back(modrm(true, reg, rm));
                    // immediate
                    std::vector<std::byte> immediate = toBytesLE(std::get<int32_t>(std::get<imm>(op2)));
                    opcode.insert(opcode.end(), immediate.begin(), immediate.end());
                // add reg, reg
                } else {
                    auto primary = std::byte(0x01);
                    std::byte mod = modrm(true, operandValueAsRegister(op2) << 3, operandValueAsRegister(op1)); 
                    opcode.push_back(primary);
                    opcode.push_back(mod);
                }
                break;
              }
            case operation::SUB:
              {
                // sub reg, imm
                if(std::holds_alternative<imm>(op2)) {
                    // primary opcode
                    auto primary = std::byte(0x81);
                    opcode.push_back(primary); 
                    // modrm
                    std::byte reg = std::byte(0b101) << 3;
                    std::byte rm = operandValueAsRegister(op1);
                    opcode.emplace_back(modrm(true, reg, rm));
                    // immediate
                    std::vector<std::byte> immediate = toBytesLE(std::get<int32_t>(std::get<imm>(op2)));
                    opcode.insert(opcode.end(), immediate.begin(), immediate.end());
                // add sub, reg
                } else {
                    auto primary = std::byte(0x29);
                    std::byte mod = modrm(true, operandValueAsRegister(op2) << 3, operandValueAsRegister(op1)); 
                    opcode.push_back(primary);
                    opcode.push_back(mod);
                }
                break;
              }
            case operation::SYSCALL:
              {
                opcode = {std::byte(0x0F), std::byte(0x05)};
                break;
              }
            case operation::JMP:
              {
                // jmp (offset relative to eip)
                if(std::holds_alternative<imm>(op1)) {
                    // TODO: other size immediates (8 and 16 bit) -- require diff primary opcodes
                    // primary opcode
                    auto primary = std::byte(0xE9);
                    opcode.push_back(primary); 
                    // immediate
                    std::vector<std::byte> immediate = toBytesLE(std::get<int32_t>(std::get<imm>(op2)));
                    opcode.insert(opcode.end(), immediate.begin(), immediate.end());
                // jmp (absolute w/ register)
                } else {
                    // TODO: other size registers (?)
                    // primary opcode
                    auto primary = std::byte(0xFF);
                    opcode.push_back(primary); 
                    // modrm
                    std::byte mod = modrm(true, std::byte(0b100 << 3), operandValueAsRegister(op1)); 
                    opcode.push_back(mod);
                }
                break;
              }
        } 

        return opcode;
    }
} // namespace opcodes
