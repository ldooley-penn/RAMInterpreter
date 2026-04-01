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
    int reg_dest = -1;
    int reg_source = -1;
    char symbol = -1;
    int line_number_dest = -1;
    
    std::string ToString() const {
        std::string result;
        switch (opcode) {
            case OpCode::ADD:
                result = "ADD r" + std::to_string(reg_dest) + ", " + std::to_string(symbol);
                break;
            case OpCode::TAIL:
                result = "TAIL r" + std::to_string(reg_dest);
                break;
            case OpCode::CLR:
                result = "CLR r" + std::to_string(reg_dest);
                break;
            case OpCode::ASSIGN:
                result = "ASSIGN r" + std::to_string(reg_dest) + ", r" + std::to_string(reg_source);
                break;
            case OpCode::GOTOA:
                result = "GOTOA " + std::to_string(line_number_dest);
                break;
            case OpCode::GOTOB:
                result = "GOTOB " + std::to_string(line_number_dest);
                break;
            case OpCode::JMPA:
                result = "JMPA r" + std::to_string(reg_source) + ", " + std::to_string(symbol) + ", " + std::to_string(line_number_dest);
                break;
            case OpCode::JMPB:
                result = "JMPB r" + std::to_string(reg_source) + ", " + std::to_string(symbol) + ", " + std::to_string(line_number_dest);
                break;
            case OpCode::CONTINUE:
                result = "CONTINUE";
                break;
        }
        if(line_number != -1) {
            result = std::to_string(line_number) + ": " + result;
        }
        return result;
    }

    void Validate(int registerCount, int symbolCount) const {
        if(reg_dest != -1 && (reg_dest <= 0 || reg_dest > registerCount)) {
            throw std::invalid_argument("Register index out of bounds for destination register for instruction: \"" + ToString() + "\"");
        }
        if(reg_source != -1 && (reg_source <= 0 || reg_source > registerCount)) {
            throw std::invalid_argument("Register index out of bounds for source register for instruction: \"" + ToString() + "\"");
        }
        if(symbol != -1 && (symbol < 0 || symbol >= symbolCount)) {
            throw std::invalid_argument("Symbol index out of bounds for instruction: \"" + ToString() + "\"");
        }
    }
};

class RAMInterpreter{
public:
    RAMInterpreter(std::vector<Instruction> instructions, int symbolCount, int numRegisters, std::vector<std::string> initialRegisters) :
        m_instructions(std::move(instructions)),
        m_symbolCount(symbolCount),
        m_registers(std::vector<std::string>(numRegisters + 1, ""))
    {
        if(initialRegisters.size() > numRegisters) {
            throw std::invalid_argument("Too many initial registers provided. Max is " + std::to_string(numRegisters));
        }
        for(size_t i = 0; i < initialRegisters.size(); i++) {
            m_registers[i + 1] = initialRegisters[i];
        }
    }

    std::string Run() {
        while(m_programCounter < m_instructions.size() && m_instructions[m_programCounter].opcode != OpCode::CONTINUE) {
            PrintState();
            const Instruction& instr = m_instructions[m_programCounter];
            switch (instr.opcode) {
                case OpCode::ADD:
                    ExecuteAdd();
                    break;
                case OpCode::TAIL:
                    ExecuteTail();
                    break;
                case OpCode::CLR:
                    ExecuteClr();
                    break;
                case OpCode::ASSIGN:
                    ExecuteAssign();
                    break;
                case OpCode::GOTOA:
                    ExecuteGotoA();
                    break;
                case OpCode::GOTOB:
                    ExecuteGotoB();
                    break;
                case OpCode::JMPA:
                    ExecuteJmpA();
                    break;
                case OpCode::JMPB:
                    ExecuteJmpB();
                    break;
                case OpCode::CONTINUE:
                    // Should never reach here due to loop condition
                    break;
                default:
                    throw std::runtime_error("Unknown opcode at line " + std::to_string(instr.line_number));
            }
        }

        PrintState();

        PrintProgram();
        return m_registers[1];
    }

private:
    void ExecuteAdd() {
        const Instruction& instr = m_instructions[m_programCounter];
        char symbolChar = instr.symbol + '0';
        m_registers[instr.reg_dest] += symbolChar;
        m_programCounter++;
    }

    void ExecuteTail() {
        const Instruction& instr = m_instructions[m_programCounter];
        std::string& reg = m_registers[instr.reg_dest];
        if(reg.size() > 0) {
            reg.erase(0, 1);
        }
        m_programCounter++;
    }

    void ExecuteClr() {
        const Instruction& instr = m_instructions[m_programCounter];
        m_registers[instr.reg_dest] = "";
        m_programCounter++;
    }

    void ExecuteAssign() {
        const Instruction& instr = m_instructions[m_programCounter];
        m_registers[instr.reg_dest] = m_registers[instr.reg_source];
        m_programCounter++;
    }

    void ExecuteGotoA() {
        // Implementation for GOTOA
        const Instruction& instr = m_instructions[m_programCounter];
        m_programCounter = FindLineNumberAbove(instr.line_number_dest);
    }

    void ExecuteGotoB() {
        // Implementation for GOTOB
        const Instruction& instr = m_instructions[m_programCounter];
        m_programCounter = FindLineNumberBelow(instr.line_number_dest);
    }

    void ExecuteJmpA() {
        // Implementation for JMPA
        const Instruction& instr = m_instructions[m_programCounter];
        char targetSymbol = instr.symbol + '0';
        if(m_registers[instr.reg_source].size() > 0 && m_registers[instr.reg_source].front() == targetSymbol) {
            m_programCounter = FindLineNumberAbove(instr.line_number_dest);
        }
        else{
            m_programCounter++;
        }
    }

    void ExecuteJmpB() {
        // Implementation for JMPB
        const Instruction& instr = m_instructions[m_programCounter];
        char targetSymbol = instr.symbol + '0';
        if(m_registers[instr.reg_source].size() > 0 && m_registers[instr.reg_source].front() == targetSymbol) {
            m_programCounter = FindLineNumberBelow(instr.line_number_dest);
        }
        else{
            m_programCounter++;
        }
    }

    int FindLineNumberAbove(int lineNumber){
        for(int i = m_programCounter - 1; i >= 0; i--) {
            if(m_instructions[i].line_number == lineNumber) {
                return i;
            }
        }
        throw std::runtime_error("Line number " + std::to_string(lineNumber) + " not found above current instruction.");
    }

    int FindLineNumberBelow(int lineNumber){
        for(size_t i = m_programCounter + 1; i < m_instructions.size(); i++) {
            if(m_instructions[i].line_number == lineNumber) {
                return i;
            }
        }
        throw std::runtime_error("Line number " + std::to_string(lineNumber) + " not found below current instruction.");
    }

    void PrintState() {
        std::cout << "PC: " << m_programCounter << " | ";
        if(m_programCounter < m_instructions.size()){
            std::cout<< m_instructions[m_programCounter].ToString() << " | ";
        }
        for(size_t i = 1; i < m_registers.size(); i++) {
            std::cout << "r" << i << ": \"" << m_registers[i] << "\" ";
        }
        std::cout << std::endl;
    }

    void PrintProgram() {
        std::cout << "Program Instructions:" << std::endl;
        for(size_t i = 0; i < m_instructions.size(); i++) {
            std::cout << "\t" << (i == m_programCounter ? "-> " : "   ") << m_instructions[i].ToString() << std::endl;
        }
    }

    std::vector<Instruction> m_instructions;
    int m_symbolCount;
    std::vector<std::string> m_registers;
    int m_programCounter = 0;
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

bool parseInstruction(std::string& line, Instruction& outInstruction){
    if(line.empty()){
        return false;
    }

    if(line.compare(0, 2, "//") == 0) {
        return false; // Skip comment lines
    }

    std::replace(line.begin(), line.end(), ',', ' ');

    std::stringstream ss(line);
    std::string token;
    ss >> token;

    // Get line number if it exists
    if(std::isdigit(token[0])) {
        outInstruction.line_number = std::stoi(token);
        ss >> token;
    }

    outInstruction.opcode = parseOpCode(token);

    switch (outInstruction.opcode) {
    case OpCode::ASSIGN: {
        // Format: assign r<reg_dest_idx>,r<reg_source_idx>
        std::string reg_dest, reg_source;
        ss >> reg_dest >> reg_source;
        outInstruction.reg_dest = std::stoi(reg_dest.substr(1));
        outInstruction.reg_source = std::stoi(reg_source.substr(1));
        break;
    }
    case OpCode::ADD: {
        // Format: add r<reg_dest_idx>,symbol
        std::string reg_dest, symbol;
        ss >> reg_dest >> symbol;
        outInstruction.reg_dest = std::stoi(reg_dest.substr(1));
        outInstruction.symbol = std::stoi(symbol);
        break;
    }
    case OpCode::GOTOA:
    case OpCode::GOTOB: {
        // Format: goto<a/b> line_number
        std::string line_number_dest;
        ss >> line_number_dest;
        outInstruction.line_number_dest = std::stoi(line_number_dest);
        break;
    }
    case OpCode::JMPA:
    case OpCode::JMPB: {
        // Format: jmp<a/b> r<reg_source_idx>,symbol,line_number
        std::string reg_source, symbol, line_number_dest;
        ss >> reg_source >> symbol >> line_number_dest;
        outInstruction.reg_source = std::stoi(reg_source.substr(1));
        outInstruction.symbol = std::stoi(symbol);
        outInstruction.line_number_dest = std::stoi(line_number_dest);
        break;
    }
    case OpCode::TAIL:
    case OpCode::CLR: {
        std::string reg_dest;
        ss >> reg_dest;
        outInstruction.reg_dest = std::stoi(reg_dest.substr(1));
        break;
    }
    case OpCode::CONTINUE:
        break;
    default:
        throw std::invalid_argument("Unknown opcode: " + token);
    } 
}

int main(int argc, char* argv[]) {
    if(argc < 5) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <symbol_count> <num_registers> <num_input_registers> [input_register 1] [input_register 2] ... [input_register N]" << std::endl;
        return 1;
    }

    std::string filepath = argv[1];

    int symbolCount;
    int registerCount;
    int inputRegisterCount;
    try
    {
        symbolCount = std::stoi(argv[2]);
        registerCount = std::stoi(argv[3]);
        inputRegisterCount = std::stoi(argv[4]);

        if(registerCount <= 0) {
            throw std::invalid_argument("Number of registers must be a positive integer.");
        }
        if(inputRegisterCount < 0 || inputRegisterCount > registerCount) {
            throw std::invalid_argument("Number of input registers must be between 0 and the total number of registers.");
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    std::vector<std::string> initialRegisters;
    for(int i = 0; i < inputRegisterCount; i++) {
        initialRegisters.push_back(argv[5 + i]);
    }

    if(!std::filesystem::exists(filepath)) {
        std::cerr << "Error: File " << filepath << " does not exist." << std::endl;
        return 1;
    }

    std::vector<Instruction> instructions;
    std::ifstream file(filepath);
    std::string line;

    bool bParsingFailed = false;
    while(std::getline(file, line)) {
        Instruction newInstruction;
        try
        {
            if(parseInstruction(line, newInstruction)) {
                newInstruction.Validate(registerCount, symbolCount);
                instructions.push_back(newInstruction);
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << "Failed to parse line: " << line << " | Error: " << e.what() << '\n';
            bParsingFailed = true;
        }
    }

    if(bParsingFailed) {
        std::cerr << "Parsing failed due to errors in the instruction file. Please fix the errors and try again." << std::endl;
        return EXIT_FAILURE;
    }

    RAMInterpreter interpreter(instructions, symbolCount, registerCount, initialRegisters);
    std::string output = interpreter.Run();

    std::cout << "Final Output: " << output << std::endl;
}