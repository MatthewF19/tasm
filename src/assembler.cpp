#include <filesystem>
#include <fstream>
#include <span>
#include <vector>

// TODO proper header files
//#include "opcode.cpp"
#include "instruction.cpp"

// write a string of hex values directly to the file
void write_hex(std::ofstream& file, std::string_view s) {
    std::vector<unsigned char> bytes;
    bytes.reserve(s.length() / 2);

    for(size_t i = 0; i < s.length(); i += 2) {
        std::string sub(s.substr(i, 2)); // O(n) copies; yuck
        bytes.push_back(static_cast<unsigned char>(std::stoul(sub, nullptr, 16)));
    } 

    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

// write a vector of hex values directly to the file
void write_hex(std::ofstream& file, std::span<std::byte> bytes) {
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

void write_header(std::ofstream& file) {
    write_hex(file, 
    "7F454C46" // ELF magic number
    "02010100" // 64 bit, little endian, always 1, ABI
    "00000000" // ABI further (0), 0s (padding)
    "00000000" // 0s (padding)
    "02003E00" // executable, AMD x86-64
    "01000000" // original ELF
    "80004000" // entry point
    "00000000" // ...
    "40000000" // start of program header table
    "00000000" // ...
    "68010000" // start of section header table 
    "00000000" // ... 
    "00000000" // ? (architecture dependent)
    "40003800" // size of header, size of program header table entry
    "01004000" // num entries in program header table, size of section header table entry
    "05000400" // num entries in section header table, index of section header table entry for section names
    );
}                 
                    
void write_program_header(std::ofstream& file) {
    write_hex(file, 
    "01000000" // this segment is loadable
    "05000000" // this segment is readable and executable
    "00000000" // segment offset
    "00000000" // ...
    "00004000" // virtual address in memory
    "00000000" // ...
    "00004000" // segment's physical address
    "00000000" // ...
    "8c000000" // size in bytes of segment in file image
    "00000000" // ...
    "8c000000" // size in bytes of segment in memory
    "00000000" // ...
    "00100000" // no alignment (?)
    "00000000" // ...
    "00000000" // padding
    "00000000" // padding
    );
}

void write_exit_program(std::ofstream& file) {
    write_hex(file, 
    "b83c000000" // mov eax, 60
    "bf04000000" // mov edi, 4
    "0f05"       // syscall
    );
}

int main() {
    // get file name
    std::string filename;
    std::cout << "enter binary name: ";
    std::cin >> filename;

    std::ofstream file(filename, std::ios::binary | std::ios::out);

    // write headers
    write_header(file);
    write_program_header(file);

    // program
    /*
    instructions::instruction eax{"mov", "eax", "60", ""};
    instructions::instruction edi{"mov", "edi", "4", ""};
    instructions::instruction add{"add", "edi", "4", ""};
    instructions::instruction add2{"add", "edi", "edi", ""};
    instructions::instruction ebx{"mov", "ebx", "5", ""};
    instructions::instruction sub{"sub", "edi", "ebx", ""};
    instructions::instruction sub2{"sub", "edi", "1", ""};
    */
    instructions::instruction pid{"mov", "eax", "39", ""};
    instructions::instruction syscall{"syscall", "", "", ""};
    instructions::instruction edi{"mov", "edi", "eax", ""};
    instructions::instruction exit{"mov", "eax", "60", ""};
    instructions::instruction syscall2{"syscall", "", "", ""};
    // meaningless operations that don't do anything in the executable because we called 
    // syscall 60 just beforehand to quit program; just testing encoding
    instructions::instruction jmp_rel{"jmp", "19", "", ""};
    instructions::instruction jmp_abs{"jmp", "ecx", "", ""};

    // write program
    std::vector<std::byte> programBytes = instructions::encodeProgram({pid, syscall, edi, exit, syscall2, jmp_rel, jmp_abs});
    write_hex(file, programBytes);

    // mark file as executable
    namespace sfs = std::filesystem;
    sfs::permissions(filename, sfs::perms::owner_exec, sfs::perm_options::add);
    file.close();
}
