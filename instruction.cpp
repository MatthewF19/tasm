#include <charconv>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

#include "opcode.cpp"

namespace instructions {
    using reg32 = opcodes::register_32;
    static const std::unordered_map<std::string_view, opcodes::operand> register_32_strings = {
        {"eax", reg32::EAX},
        {"ecx", reg32::ECX}, 
        {"edx", reg32::EDX}, 
        {"ebx", reg32::EBX}, 
        {"esp", reg32::ESP}, 
        {"ebp", reg32::EBP}, 
        {"esi", reg32::ESI}, 
        {"edi", reg32::EDI}, 
        {"r8d", reg32::R8D}, 
        {"r9d", reg32::R9D}, 
        {"r10d", reg32::R10D}, 
        {"r11d", reg32::R11D}, 
        {"r12d", reg32::R12D}, 
        {"r13d", reg32::R13D}, 
        {"r14d", reg32::R14D}, 
        {"r15d", reg32::R15D}};

    opcodes::operation parseOperation(std::string_view s) {
        if(s == "mov") { return opcodes::operation::MOV; }
        if(s == "add") { return opcodes::operation::ADD; }
        if(s == "sub") { return opcodes::operation::SUB; }
        if(s == "syscall") { return opcodes::operation::SYSCALL; }
        if(s == "jmp") { return opcodes::operation::JMP; }
        else { throw std::invalid_argument("not a valid operation"); }
    }

    opcodes::operand parseOperand(std::string_view s) {
        // TODO check for 16/64 bit and parse immediate appropriately
        // operand is register
        if(register_32_strings.count(s) != 0) { return register_32_strings.at(s); }
        // operand is immediate
        int immediate;
        std::from_chars(s.data(), s.data()+s.size(), immediate);
        return immediate;
    }

    class instruction {
        opcodes::operation operation;
        opcodes::operand op1;
        opcodes::operand op2;
        opcodes::operand op3;
        opcodes::bitsize bitsize;
    public:
        instruction(std::string_view operation, 
                    std::string_view op1, 
                    std::string_view op2, 
                    std::string_view op3,
                    opcodes::bitsize bitsize = opcodes::bitsize::b32)
          : operation(parseOperation(operation)),
            op1(parseOperand(op1)), 
            op2(parseOperand(op2)),
            op3(parseOperand(op3)),
            bitsize(bitsize) {}

        const opcodes::operation& getOperation() { return operation; }
        const opcodes::operand& getOp1() { return op1; }
        const opcodes::operand& getOp2() { return op2; }
        const opcodes::operand& getOp3() { return op3; }
        const opcodes::bitsize& getBitsize() { return bitsize; }

        std::vector<std::byte> encode() const {
            return opcodes::encode(operation, op1, op2, op3, bitsize);
        }
    };

    std::vector<std::byte> encodeProgram(const std::vector<instruction>& instructions) {
        std::vector<std::byte> encodings;
        for(const auto& i : instructions) {
            std::vector<std::byte> bytes = i.encode();
            encodings.insert(encodings.end(), bytes.begin(), bytes.end());
        }

        return encodings;
    }
} // namespace instructions
