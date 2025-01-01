#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
using namespace std;

vector<int> registers(32, 1); // 預設值為 1
vector<int> memory(32, 1);    // 預設值為 1

int PC = 0;
int cycle = 1;

struct Instruction {
    string opcode;   // lw, sw, add, sub, beq
    int rs, rt, rd; // reg編號
    int immediate;
    string stage="-"; // IF, ID, EX, MEM, WB
};
vector<Instruction> instructionMemory; // 指令記憶體

struct PipelineRegister {
    Instruction ins; // 目前指令
    vector<int> controlSignals; // 0: ALUSrc, 1: RegDst, 2: Branch, 3: MemRead, 4: MemWrite, 5: RegWrite, 6: MemtoReg
    bool valid = false; // 看reg能不能用ins
};

PipelineRegister IF_ID, ID_EX, EX_MEM, MEM_WB;

void init() {
    registers[0] = 0; // $zero = 0
}

// 在ID要設定control signal，參考小考二的圖和你的答案
void setControlSignals(PipelineRegister &reg, string &opcode) {
    reg.controlSignals.assign(7, 0); // 7個控制都先設0，有要1的再改
    if(opcode == "add" || opcode == "sub") {
        reg.controlSignals[1] = 1;// RegDst
        reg.controlSignals[5] = 1;// RegWrite
    } else if(opcode == "lw") {
        reg.controlSignals[0] = 1;// ALUSrc
        reg.controlSignals[3] = 1;// MemRead
        reg.controlSignals[5] = 1;// RegWrite
        reg.controlSignals[6] = 1;// MemtoReg
    } else if(opcode == "sw") {
        reg.controlSignals[0] = 1;// ALUSrc
        reg.controlSignals[4] = 1;// MemWrite
    } else if(opcode == "beq") {
        reg.controlSignals[2] = 1;// Branch
    }
}

void IF() {
    if(IF_ID.valid) return; // 如果IF_ID已經有指令，則跳過

    if(PC < instructionMemory.size()) { // 如果PC還在指令範圍內
        IF_ID.ins = instructionMemory[PC]; // 把指令放到IF_ID
        IF_ID.valid = true; // 設置IF_ID有效
        IF_ID.ins.stage = "IF"; // 設定這條指令的階段為 IF
        instructionMemory[PC].stage = "IF"; // 更新instructionMemory中的指令階段
        //PC++; // 增加PC以指向下一條指令
    }
    PC++;
}

void ID() {
    if(!IF_ID.valid) return; // 如果IF_ID沒有指令，則跳過

    ID_EX.ins = IF_ID.ins; // 把IF_ID的指令傳到ID_EX
    ID_EX.valid = true; // 設置ID_EX有效
    instructionMemory[PC-1].stage = "ID"; // 更新instructionMemory中的指令階段
    IF_ID.valid = false; // 用完了IF_ID的指令
}

void EX() {
    if(!ID_EX.valid) return; // 如果ID_EX沒有指令，則跳過

    EX_MEM.ins = ID_EX.ins; // 把ID_EX的指令傳到EX_MEM
    EX_MEM.valid = true; // 設置EX_MEM有效
    instructionMemory[PC-2].stage = "EX"; // 更新instructionMemory中的指令階段
    ID_EX.valid = false; // 用完了ID_EX的指令

    // 執行指令（計算等）
    if(EX_MEM.ins.opcode == "add") {
        registers[EX_MEM.ins.rd] = registers[EX_MEM.ins.rs] + registers[EX_MEM.ins.rt]; // rd = rs + rt
    } else if(EX_MEM.ins.opcode == "sub") {
        registers[EX_MEM.ins.rd] = registers[EX_MEM.ins.rs] - registers[EX_MEM.ins.rt]; // rd = rs - rt
    } else if(EX_MEM.ins.opcode == "lw" || EX_MEM.ins.opcode == "sw") {
        EX_MEM.ins.immediate = registers[EX_MEM.ins.rs] + (EX_MEM.ins.immediate<<2); // immediate = rs + (immediate<<2)
    }
}

void MEM() {
    if(!EX_MEM.valid) return; // 如果EX_MEM沒有指令，則跳過
    MEM_WB.ins = EX_MEM.ins; // 把EX_MEM的指令傳到MEM_WB
    MEM_WB.valid = true; // 設置MEM_WB有效

    instructionMemory[PC-3].stage = "MEM"; // 更新instructionMemory中的指令階段
    EX_MEM.valid = false; // 用完了EX_MEM的指令
    // 根據指令類型執行操作
    if(MEM_WB.ins.opcode == "lw") {
        registers[MEM_WB.ins.rt] = memory[MEM_WB.ins.immediate]; // rt = mem[immediate]
    } else if(MEM_WB.ins.opcode == "sw") {
        memory[MEM_WB.ins.immediate] = registers[MEM_WB.ins.rt]; // mem[immediate] = rt
    }
}

void WB() {
    if(!MEM_WB.valid) return; // 如果MEM_WB沒有指令，則跳過
    instructionMemory[PC-4].stage = "WB"; // 更新instructionMemory中的指令階段
    // 只有這三個有WB
    if(MEM_WB.ins.opcode == "add" || MEM_WB.ins.opcode == "sub") {
        registers[MEM_WB.ins.rd] = registers[MEM_WB.ins.rs];
    } else if(MEM_WB.ins.opcode == "lw") {
        registers[MEM_WB.ins.rt] = registers[MEM_WB.ins.immediate];
    }
    MEM_WB.valid = false; // 用完了
}

void readInput(const string& filename) {

    ifstream infile(filename);
    if(!infile) {
        cout << "Cannot open file.\n";
        return;
    }
    string line;
    while (getline(infile, line)) { 
        Instruction ins;
        size_t pos = 0;
        // 讀取操作碼
        pos = line.find(" ");
        ins.opcode = line.substr(0, pos);
        line.erase(0, pos + 1);  
        //處理字串，得到暫存器編號
        if (ins.opcode == "lw" || ins.opcode == "sw") {
            string rt, rs, immediatePart;

            pos = line.find(","); 
            rt = line.substr(0, pos); 
            ins.rt = stoi(rt.substr(1)); 
            line.erase(0, pos + 2);

            pos = line.find("(");
            immediatePart = line.substr(0, pos); 
            ins.immediate = stoi(immediatePart); 
            line.erase(0, pos + 1); 

            pos = line.find(")");
            rs = line.substr(0, pos); 
            ins.rs = stoi(rs.substr(1)); 

            cout << "Parsed I instruction - opcode: " << ins.opcode 
                << ", rt: " << ins.rt 
                << ", rs: " << ins.rs 
                << ", immediate: " << ins.immediate << endl;
        } else if (ins.opcode == "beq") {
            string rs, rt, immediatePart;

            pos = line.find(",");
            rs = line.substr(0, pos); 
            line.erase(0, pos + 2);

            pos = line.find(","); 
            rt = line.substr(0, pos); 
            line.erase(0, pos + 2); 

            immediatePart = line; 
            ins.immediate = stoi(immediatePart);

            ins.rs = stoi(rs.substr(1));
            ins.rt = stoi(rt.substr(1));

            cout << "Parsed beq instruction - opcode: " << ins.opcode 
                << ", rs: " << ins.rs 
                << ", rt: " << ins.rt 
                << ", immediate: " << ins.immediate << endl;
        } else if (ins.opcode == "add" || ins.opcode == "sub") {
            string rd, rs, rt;

            pos = line.find(","); 
            rd = line.substr(0, pos); 
            line.erase(0, pos + 2); 

            pos = line.find(","); 
            rs = line.substr(0, pos); 
            line.erase(0, pos + 2); 

            rt = line; 
            ins.rd = stoi(rd.substr(1)); 
            ins.rs = stoi(rs.substr(1)); 
            ins.rt = stoi(rt.substr(1)); 

            cout << "Parsed R instruction - opcode: " << ins.opcode 
                << ", rd: " << ins.rd 
                << ", rs: " << ins.rs 
                << ", rt: " << ins.rt << endl;
        }
        instructionMemory.push_back(ins);
    }
}

void Output() {
    cout << "Cycle " << cycle << endl;
    for(auto &i: instructionMemory) {
        if(i.stage != "-") cout << i.opcode << " - Stage: " << i.stage << endl;
    }
}

void simulate() {
    while(true) {
        // 每次迴圈代表一個cycle
        WB();
        MEM();
        EX();
        ID();
        IF();
        Output();
        if(!IF_ID.valid && !ID_EX.valid && !EX_MEM.valid && !MEM_WB.valid && PC >= instructionMemory.size()) break; // 如果全部都沒有指令了就結束
        cycle++;
    }
}


int main() {
    init();
    readInput("../inputs/tt.txt");
    simulate();
    return 0;
}