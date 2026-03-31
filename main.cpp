#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <filesystem>
#include <sstream>

enum class OpCode{
    ADD,
    TAIL,
    CLR,
    ASSIGN,
    GOTOA,
    GOTOB,
    JMPA,
    JMPB,
    CONTINUE
};

struct Instruction {
    OpCode opcode;
    int line_number = -1;
    int reg_source = -1;
    int reg_dest = -1;
    char symbol = -1;
    int line_number_dest = -1;
};

OpCode parseOpCode(const std::string& str) {
    if (str == "add") return OpCode::ADD;
    if (str == "tail") return OpCode::TAIL;
    if (str == "clr") return OpCode::CLR;
    if (str == "assign") return OpCode::ASSIGN;
    if (str == "gotoa") return OpCode::GOTOA;
    if (str == "gotob") return OpCode::GOTOB;
    if (str == "jmpa") return OpCode::JMPA;
    if (str == "jmpb") return OpCode::JMPB;
    if (str == "continue") return OpCode::CONTINUE;
    throw std::invalid_argument("Invalid opcode: " + str);
}

int main(int argc, char* argv[]) {
    if(argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <num_registers>" << std::endl;
        return 1;
    }

    std::string filepath = argv[1];

    int num_registers;
    try
    {
        num_registers = std::stoi(argv[2]);

        if(num_registers <= 0) {
            throw std::invalid_argument("Number of registers must be a positive integer.");
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    

    if(!std::filesystem::exists(filepath)) {
        std::cerr << "Error: File " << filepath << " does not exist." << std::endl;
        return 1;
    }
    
    std::vector<std::string> registers(num_registers + 1);

    std::vector<Instruction> instructions;
    std::ifstream file(filepath);
    std::string line;

    std::vector<char> symbols;

    while(std::getline(file, line)) {
        if(line.empty()) continue;

        std::replace(line.begin(), line.end(), ',', ' ');

        if(line[0] == 'r' && line.find('=') != std::string::npos) {
            size_t eq_pos = line.find('=');
            int reg_index = std::stoi(line.substr(1, eq_pos - 1));
            if(reg_index < 1 || reg_index > num_registers) {
                std::cerr << "Error: Register index out of bounds:" << reg_index << std::endl;
                return EXIT_FAILURE;
            }
            registers[reg_index] = line.substr(eq_pos + 1);
            continue;
        }

        if(line.substr(0, 2) == "S="){
            std::string alphabet = line.substr(2);
            symbols.assign(alphabet.begin(), alphabet.end());
            continue;
        }

        std::stringstream ss(line);
        std::string token;
        ss >> token;

        Instruction instr;

        // Get line number if it exists
        if(std::isdigit(token[0])) {
            instr.line_number = std::stoi(token);
            ss >> token;
        }

        instr.opcode = parseOpCode(token);

        switch (instr.opcode) {
        case OpCode::ASSIGN: {
            // Format: assign r<reg_dest_idx>,r<reg_source_idx>
            std::string reg_dest, reg_source;
            ss >> reg_dest >> reg_source;
            instr.reg_dest = std::stoi(reg_dest.substr(1));
            instr.reg_source = std::stoi(reg_source.substr(1));
            break;
        }
        case OpCode::ADD: {
            // Format: add r<reg_dest_idx>,symbol
            std::string reg_dest, symbol;
            ss >> reg_dest >> symbol;
            instr.reg_dest = std::stoi(reg_dest.substr(1));
            int symbol_idx = std::stoi(symbol);
            if(symbol_idx < 0 || symbol_idx >= symbols.size()) {
                std::cerr << "Error: Symbol index out of bounds at line " << instr.line_number << std::endl;
                return EXIT_FAILURE;
            }
            instr.symbol = symbols.at(symbol_idx);
            break;
        }
        case OpCode::GOTOA:
        case OpCode::GOTOB: {
            // Format: goto<a/b> line_number
            std::string line_number_dest;
            ss >> line_number_dest;
            instr.line_number_dest = std::stoi(line_number_dest);
            break;
        }
        case OpCode::JMPA:
        case OpCode::JMPB: {
            // Format: jmp<a/b> r<reg_source_idx>,symbol,line_number
            std::string reg_source, symbol, line_number_dest;
            ss >> reg_source >> symbol >> line_number_dest;
            instr.reg_source = std::stoi(reg_source.substr(1));
            int symbol_idx = std::stoi(symbol);
            if(symbol_idx < 0 || symbol_idx >= symbols.size()) {
                std::cerr << "Error: Symbol index out of bounds at line " << instr.line_number << std::endl;
                return EXIT_FAILURE;
            }
            instr.symbol = symbols.at(symbol_idx);
            instr.line_number_dest = std::stoi(line_number_dest);
            break;
        }
        case OpCode::TAIL:
        case OpCode::CLR: {
            std::string reg_dest;
            ss >> reg_dest;
            instr.reg_dest = std::stoi(reg_dest.substr(1));
            break;
        }
        case OpCode::CONTINUE:
            break;
        default:
            std::cerr << "Error: Unknown opcode " << std::endl;
            return EXIT_FAILURE;
        } 

        instructions.push_back(instr);
    }

    int current_instruction_index = 0;
    while(instructions[current_instruction_index].opcode != OpCode::CONTINUE) {
        Instruction& current_instruction = instructions[current_instruction_index];

        switch (current_instruction.opcode) {
        case OpCode::ADD:
            registers[current_instruction.reg_dest].push_back(current_instruction.symbol);
            current_instruction_index++;
            break;
        case OpCode::TAIL:
            registers[current_instruction.reg_dest] = registers[current_instruction.reg_dest].substr(1);
            current_instruction_index++;
            break;
        case OpCode::CLR:
            registers[current_instruction.reg_dest] = "";
            current_instruction_index++;
            break;
        case OpCode::ASSIGN:
            registers[current_instruction.reg_dest] = registers[current_instruction.reg_source];
            current_instruction_index++;
            break;
        case OpCode::GOTOA:
            break;
        case OpCode::GOTOB:
            break;
        case OpCode::JMPA:
            break;
        case OpCode::JMPB:
            break;
        default:
            std::cerr << "Error: Unknown opcode during execution" << std::endl;
            return EXIT_FAILURE;
        }
    }
}