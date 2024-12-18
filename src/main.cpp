#include <iostream>
#include <string>
#include <vector>
using namespace std;

vector<int> registers(32, 1); // 預設值為 1
vector<int> memory(32, 1);    // 預設值為 1

struct Instruction {
    string opcode;   // lw, sw, add, sub, beq
    int rs, rt, rd;
    int immediate;
};

struct PipelineRegister {
    Instruction ins;
    int ALUSrc, RegDst, Branch, MemRead, MemWrite, RegWrite, MemtoReg;
    bool valid; // 看能不能用
};

PipelineRegister IF_ID, ID_EX, EX_MEM, MEM_WB;

void init() {
    registers[0] = 0; // $zero = 0
}

void IF() {
    if(IF_ID.valid) return; // 如果IF_ID有東西就不做

    IF_ID.valid = true; // IF_ID在IF之後才會有指令
}

void ID() {
    if(!IF_ID.valid) return; // 如果IF_ID沒有東西就不做
    ID_EX.ins = IF_ID.ins; // 把IF_ID的指令放到ID_EX
    ID_EX.valid = true; // ID_EX在ID之後才會有指令
    IF_ID.valid = false; // IF_ID在ID之後才會有指令
}

void EX() {
    if(!ID_EX.valid) return; // 如果ID_EX沒有東西就不做
    EX_MEM.ins = ID_EX.ins; // 把ID_EX的指令放到EX_MEM
    EX_MEM.valid = true; // EX_MEM在EX之後才會有指令
    ID_EX.valid = false; // ID_EX在EX之後才會有指令
}

void MEM() {
    if(!EX_MEM.valid) return; // 如果EX_MEM沒有東西就不做
    MEM_WB.ins = EX_MEM.ins; // 把EX_MEM的指令放到MEM_WB
    MEM_WB.valid = true; // MEM_WB在MEM之後才會有指令
    EX_MEM.valid = false; // EX_MEM在MEM之後才會有指令
}

void WB() {
    if(!MEM_WB.valid) return; // 如果MEM_WB沒有東西就不做
    MEM_WB.valid = false; // MEM_WB在WB之後才會有指令
}

int main() {
    init();
    return 0;
}