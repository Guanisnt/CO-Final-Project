#include <iostream>
#include <string>
#include <vector>
#include <fstream>
using namespace std;

vector<int> registers(32, 1); // 預設值為 1
vector<int> memory(32, 1);    // 預設值為 1

int PC = 0;
int cycle = 1;

struct Instruction {
    string opcode;   // lw, sw, add, sub, beq
    int rs, rt, rd; // reg編號
    int immediate;
};
vector<Instruction> instructionMemory; // 指令記憶體

struct PipelineRegister {
    Instruction ins; // 目前指令
    int ALUSrc, RegDst, Branch, MemRead, MemWrite, RegWrite, MemtoReg; // control signal
    bool valid; // 看能不能用
};

PipelineRegister IF_ID, ID_EX, EX_MEM, MEM_WB;

void init() {
    registers[0] = 0; // $zero = 0
}

void IF() {
    if(IF_ID.valid) return; // 如果IF_ID有指令了就不要再fetch

    if(PC < instructionMemory.size()) {
        IF_ID.ins = instructionMemory[PC]; // 把指令放到IF_ID
        IF_ID.valid = true; // IF_ID在IF有指令了
        PC++; // 應該是+4，但是這裡指令用vector存所以+1
    }
}

void ID() {
    if(!IF_ID.valid) return; // 如果IF_ID沒有東西就不做
    ID_EX.ins = IF_ID.ins; // 把IF_ID的指令船到ID_EX

    // control signal設定
    if(ID_EX.ins.opcode == "add" || ID_EX.ins.opcode == "sub") {
        ID_EX.RegDst = 1; ID_EX.ALUSrc = 0; ID_EX.MemtoReg = 0; ID_EX.MemRead = 0; ID_EX.MemWrite = 0; ID_EX.RegWrite = 1; ID_EX.Branch = 0;
    } else if(ID_EX.ins.opcode == "lw") {
        ID_EX.RegDst = 0; ID_EX.ALUSrc = 1; ID_EX.MemtoReg = 1; ID_EX.MemRead = 1; ID_EX.MemWrite = 0; ID_EX.RegWrite = 1; ID_EX.Branch = 0;
    } else if(ID_EX.ins.opcode == "sw") {
        ID_EX.RegDst = 0; ID_EX.ALUSrc = 1; ID_EX.MemtoReg = 0; ID_EX.MemRead = 0; ID_EX.MemWrite = 1; ID_EX.RegWrite = 0; ID_EX.Branch = 0;
    } else if(ID_EX.ins.opcode == "beq") {
        ID_EX.RegDst = 0; ID_EX.ALUSrc = 0; ID_EX.MemtoReg = 0; ID_EX.MemRead = 0; ID_EX.MemWrite = 0; ID_EX.RegWrite = 0; ID_EX.Branch = 1;
    }
}

void EX() {
    if(!ID_EX.valid) return; // 如果ID_EX沒有東西就不做
    EX_MEM.ins = ID_EX.ins; // 把ID_EX的指令傳到EX_MEM
    EX_MEM.valid = true; // EX_MEM在EX之後才會有指令
    ID_EX.valid = false; // 用完了

    if(EX_MEM.ins.opcode == "add") {
        registers[EX_MEM.ins.rd] = registers[EX_MEM.ins.rs] + registers[EX_MEM.ins.rt];
    } else if(EX_MEM.ins.opcode == "sub") {
        registers[EX_MEM.ins.rd] = registers[EX_MEM.ins.rs] - registers[EX_MEM.ins.rt];
    } else if(EX_MEM.ins.opcode == "lw" || EX_MEM.ins.opcode == "sw") {
        EX_MEM.ins.immediate = registers[EX_MEM.ins.rs] + EX_MEM.ins.immediate<<2;
    }
}

// lw rt, immediate(rs), sw rt, immediate(rs)
void MEM() {
    if(!EX_MEM.valid) return; // 如果EX_MEM沒有東西就不做
    MEM_WB.ins = EX_MEM.ins; // 把EX_MEM的指令傳到MEM_WB
    MEM_WB.valid = true; // MEM_WB在MEM之後才會有指令
    EX_MEM.valid = false; // 用完了

    if(MEM_WB.ins.opcode == "lw") {
        registers[MEM_WB.ins.rt] = memory[MEM_WB.ins.immediate];
    } else if(MEM_WB.ins.opcode == "sw") {
        memory[MEM_WB.ins.immediate] = registers[MEM_WB.ins.rt];
    }
}

void WB() {
    if(!MEM_WB.valid) return; // 如果MEM_WB沒有東西就不做
    if(MEM_WB.ins.opcode == "add" || MEM_WB.ins.opcode == "sub" || MEM_WB.ins.opcode == "lw") {
        // 還沒想
    }
    MEM_WB.valid = false; // 用完了
}

void readInput(const string& filename) {
    ifstream infile(filename);
    string line;
    while(getline(infile, line)) {
        // 太煩了
    }
}

int main() {
    init();

    while(true) {
        // 每次迴圈代表一個cycle
        WB();
        MEM();
        EX();
        ID();
        IF();
        if(!IF_ID.valid && !ID_EX.valid && !EX_MEM.valid && !MEM_WB.valid && PC >= instructionMemory.size()) break;
        cycle++;
    }

    return 0;
}