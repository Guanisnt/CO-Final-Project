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
    if(IF_ID.valid) return; // 如果IF_ID有指令了就不要再fetch

    if(PC < instructionMemory.size()) { // 如果PC還在指令範圍內才能坐下去
        IF_ID.ins = instructionMemory[PC]; // 把指令放到IF_ID
        IF_ID.valid = true; // IF_ID在IF有指令了
        PC++; // 應該是+4，但是這裡指令用vector存所以+1
    }
}

void ID() {
    if(!IF_ID.valid) return; // 如果IF_ID沒有東西就不做
    ID_EX.ins = IF_ID.ins; // 把IF_ID的指令船到ID_EX

    // control signal設定
    setControlSignals(ID_EX, ID_EX.ins.opcode); // 根據opcode設定control signal\

    ID_EX.valid = true; // ID_EX在ID之後才會有指令
    IF_ID.valid = false; // 用完了
}

void EX() {
    if(!ID_EX.valid) return; // 如果ID_EX沒有東西就不做
    EX_MEM.ins = ID_EX.ins; // 把ID_EX的指令傳到EX_MEM
    EX_MEM.valid = true; // EX_MEM在EX之後才會有指令
    ID_EX.valid = false; // 用完了

    // 先做一般的ALU運算
    if(EX_MEM.ins.opcode == "add") {
        registers[EX_MEM.ins.rd] = registers[EX_MEM.ins.rs] + registers[EX_MEM.ins.rt]; // rd = rs + rt
    } else if(EX_MEM.ins.opcode == "sub") {
        registers[EX_MEM.ins.rd] = registers[EX_MEM.ins.rs] - registers[EX_MEM.ins.rt]; // rd = rs - rt
    } else if(EX_MEM.ins.opcode == "lw" || EX_MEM.ins.opcode == "sw") {
        EX_MEM.ins.immediate = registers[EX_MEM.ins.rs] + (EX_MEM.ins.immediate<<2); // immediate = rs + (immediate<<2)
    }

    // 處理 beq (在 EX 階段真正判斷是否跳)
    if (EX_MEM.ins.opcode == "beq") {
        // 若 rs == rt，branch taken
        if (registers[EX_MEM.ins.rs] == registers[EX_MEM.ins.rt]) {
            // 在 IF 階段已經 PC++，所以這裡做 PC += (offset - 1) 才能正確跳到目標
            PC += (EX_MEM.ins.immediate - 1);

            // Flush：清除未來 pipeline 階段中「已經抓到但還沒執行完」的指令
            IF_ID.valid = false;
            ID_EX.valid = false;
        }
        // 若 rs != rt，則 branch not taken，什麼都不做
    }
}

// lw rt, immediate(rs), sw rt, immediate(rs)
void MEM() {
    if(!EX_MEM.valid) return; // 如果EX_MEM沒有東西就不做
    MEM_WB.ins = EX_MEM.ins; // 把EX_MEM的指令傳到MEM_WB
    MEM_WB.valid = true; // MEM_WB在MEM之後才會有指令
    EX_MEM.valid = false; // 用完了

    if(MEM_WB.ins.opcode == "lw") {
        registers[MEM_WB.ins.rt] = memory[MEM_WB.ins.immediate]; // rt = mem[immediate]
    } else if(MEM_WB.ins.opcode == "sw") {
        memory[MEM_WB.ins.immediate] = registers[MEM_WB.ins.rt]; // mem[immediate] = rt
    }
}

void WB() {
    if(!MEM_WB.valid) return; // 如果MEM_WB沒有東西就不做
    
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
    while (getline(infile, line)) { // 一行一行讀取指令
        Instruction ins;
        size_t pos = 0;
        // 讀取操作碼
        pos = line.find(" ");
        ins.opcode = line.substr(0, pos);
        line.erase(0, pos + 1);  // 去掉操作碼部分

        if (ins.opcode == "lw" || ins.opcode == "sw") {
            string rt, rs, immediatePart;

            // 讀取 $rt
            pos = line.find(","); // 找到逗號的位置
            rt = line.substr(0, pos); // 提取出 $rt 部分
            ins.rt = stoi(rt.substr(1)); // 去掉 "$" 並轉換為整數
            line.erase(0, pos + 2); // 去掉已經讀取過的部分

            // 讀取 offset(base)，直到 "("
            pos = line.find("(");
            immediatePart = line.substr(0, pos); // 提取 offset 部分
            ins.immediate = stoi(immediatePart); // 轉換為整數
            line.erase(0, pos + 1); // 去掉已經讀取過的部分

            // 讀取 base，直到 ")"
            pos = line.find(")");
            rs = line.substr(0, pos); // 提取 base 寄存器
            ins.rs = stoi(rs.substr(1)); // 去掉 "$" 並轉換為整數

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

            // 讀取 rd
            pos = line.find(","); // 找到逗號的位置
            rd = line.substr(0, pos); // 提取出 rd 部分
            line.erase(0, pos + 2); // 去掉已經讀取過的部分

            // 讀取 rs
            pos = line.find(","); // 找到第二個逗號的位置
            rs = line.substr(0, pos); // 提取出 rs 部分
            line.erase(0, pos + 2); // 去掉已經讀取過的部分

            // 讀取 rt
            rt = line; // 剩下的部分即為 rt
            ins.rd = stoi(rd.substr(1)); // 去掉 "$" 並轉換為整數
            ins.rs = stoi(rs.substr(1)); // 去掉 "$" 並轉換為整數
            ins.rt = stoi(rt.substr(1)); // 去掉 "$" 並轉換為整數

            cout << "Parsed R instruction - opcode: " << ins.opcode 
                << ", rd: " << ins.rd 
                << ", rs: " << ins.rs 
                << ", rt: " << ins.rt << endl;
        }
        instructionMemory.push_back(ins);
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
        if(!IF_ID.valid && !ID_EX.valid && !EX_MEM.valid && !MEM_WB.valid && PC >= instructionMemory.size()) break; // 如果全部都沒有指令了就結束
        cycle++;
    }
}

int main() {
    init();
    readInput("../inputs/test4.txt");
    simulate();
    return 0;
}