#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
using namespace std;

vector<int> registers(32, 1); // 預設值為 1
vector<int> memory(32, 1);    // 預設值為 1

int tmp_EX=0;
int tmp_MEM=0;
int tmp_WB=0;

//A代表rs、B代表rt
//2 要從前指令拿 (在EX_MEM) EX hazard
//1 要從前前指令拿 (在MEM_WB) MEM hazard
//3 代表前前指令是lw (在MEM_WB) Load-Use hazard 不用stall
//3 & 1(都是前前指令)的差別是，3是要判斷rt是不是目前指令的rs或rt，因為lw是寫入rt
int forwardA=0, forwardB=0;

int PC = 0;
int cycle = 1;
int stall = 0;
bool stall_IF=false, stall_ID=false,stall_EX=false;
bool continue_stall=false;

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
    //if (IF_ID.valid || ID_EX.valid && ID_EX.controlSignals[2]) return; //這會有問題，因為ID_EX.controlSignals[2]是Branch，不是Branch的時候也要fetch
    if(IF_ID.valid){
        return; // 如果IF_ID有指令了就不要再fetch
    }

    if(PC < instructionMemory.size()) { // 如果PC還在指令範圍內
        IF_ID.ins = instructionMemory[PC]; // 把指令放到IF_ID
        IF_ID.valid = true; // 設置IF_ID有效
        if(stall!=1 && !continue_stall) PC++; // 增加PC以指向下一條指令
        cout<< IF_ID.ins.opcode << ": IF" << endl;
    }
}

void ID() {
    if(!IF_ID.valid) return; // 如果IF_ID沒有東西就不做

    if(stall!=1){
        // cout<<"not stall\n";
        ID_EX = IF_ID; // 把IF_ID的指令船到ID_EX
        ID_EX.valid = true; // 設置ID_EX有效
        IF_ID.valid = false; // 用完了IF_ID的指令
    }

        // control signal設定
        setControlSignals(ID_EX, ID_EX.ins.opcode); // 根據opcode設定control signal


        //判斷需不需要Forwarding Stall
        if(ID_EX.ins.opcode == "add" || ID_EX.ins.opcode == "sub") {
            
            //EX hazard 從前一指令 EX_MEM階段拿
            if(EX_MEM.controlSignals[5]==1 && EX_MEM.ins.rd && EX_MEM.ins.rd == ID_EX.ins.rs){
                forwardA=2;
                continue_stall = false;
            }else if(EX_MEM.controlSignals[5]==1 && EX_MEM.ins.rd && EX_MEM.ins.rd == ID_EX.ins.rt){
                forwardB=2;
                continue_stall = false;
            }

            //MEM hazard 從前前指令 controlSignals[5] => RegWrite
            if( MEM_WB.controlSignals[5] == 1 && MEM_WB.ins.rd &&
                !(EX_MEM.controlSignals[5]==1 && EX_MEM.ins.rd && EX_MEM.ins.rd == ID_EX.ins.rs)
                && MEM_WB.ins.rd == ID_EX.ins.rs){
                forwardA=1;
                continue_stall = false;
            }else if(MEM_WB.controlSignals[5] == 1 && MEM_WB.ins.rd &&
                    !(EX_MEM.controlSignals[5]==1 && EX_MEM.ins.rd && EX_MEM.ins.rd == ID_EX.ins.rt)
                    && MEM_WB.ins.rd == ID_EX.ins.rt){
                forwardB=1;
                continue_stall = false;
            }

            //判斷前指令是不是lw controlSignals[6] => MemtoReg
            if(EX_MEM.controlSignals[6] == 1 && EX_MEM.ins.rt == ID_EX.ins.rs){
                if(stall==0) stall = 2;
                else continue_stall = true;
            }else if(EX_MEM.controlSignals[6] == 1 && EX_MEM.ins.rt == ID_EX.ins.rt){
                if(stall==0) stall = 2;
                else continue_stall = true;
            }
            
            //判斷前前指令是不是lw controlSignals[6] => MemtoReg
            if(MEM_WB.controlSignals[6] == 1 && MEM_WB.ins.rt == ID_EX.ins.rs){
                forwardA=3;
                continue_stall = false;
            }else if(MEM_WB.controlSignals[6] == 1 && MEM_WB.ins.rt == ID_EX.ins.rt){
                forwardB=3;
                continue_stall = false;
            }
        }else if(ID_EX.ins.opcode == "sw" ) {
            if(EX_MEM.controlSignals[5]==1 && EX_MEM.ins.rd && EX_MEM.ins.rd == ID_EX.ins.rt ){
                tmp_EX=tmp_WB;
                continue_stall = false;
            }
        }else if(ID_EX.ins.opcode == "beq"){
            //前一個指令為R-format，有beq要比較的暫存器，並且會經過ALU
            if(EX_MEM.controlSignals[5] == 1 && EX_MEM.ins.rd && (EX_MEM.ins.rd == ID_EX.ins.rs || EX_MEM.ins.rd == ID_EX.ins.rt)){
                if(stall==0) stall = 2;
                else continue_stall=true;
            //前一個指令為lw，存入暫存器rt是beq要比較的暫存器之一
            }else if(EX_MEM.controlSignals[3] == 1 && (EX_MEM.ins.rt == ID_EX.ins.rt || EX_MEM.ins.rt == ID_EX.ins.rs)){
                if(stall==0) stall = 2;
                else continue_stall=true;
            //前前指令為lw，存入暫存器rt是beq要比較的暫存器之一
            }else if(MEM_WB.controlSignals[3] == 1 && (MEM_WB.ins.rt == ID_EX.ins.rt || MEM_WB.ins.rt == ID_EX.ins.rs)){
                if(stall==0) stall = 2;
                else continue_stall=true;
            }
        }
    cout << ID_EX.ins.opcode << ": ID" << endl;
}

void EX() {
    if(!ID_EX.valid || stall || continue_stall) return; // 如果ID_EX沒有東西就不做

    EX_MEM.ins.immediate = registers[EX_MEM.ins.rs] + EX_MEM.ins.immediate;
    EX_MEM = ID_EX; // 把ID_EX的指令傳到EX_MEM
    EX_MEM.valid = true; // EX_MEM在EX之後才會有指令
    ID_EX.valid = false; // 用完了


    if(EX_MEM.ins.opcode == "add" || EX_MEM.ins.opcode == "sub") {
        //計算
        //先設tmp_rs，tmp_rt為沒有forwarding 直接從register拿資料
        int tmp_rs = registers[EX_MEM.ins.rs], tmp_rt = registers[EX_MEM.ins.rt];

        // rs從前指令(在EX_MEM)
        if(forwardA == 2) tmp_rs = tmp_MEM; 
        // rs從前前指令(在MEM_WB)
        else if(forwardA == 1 || forwardA ==3) tmp_rs = tmp_WB;

        // rt從前指令(在EX_MEM)
        if(forwardB == 2) {
            if(EX_MEM.ins.opcode == "add") tmp_rt = tmp_MEM; 
            else tmp_rt = - tmp_MEM;
        }
        // rt從前前指令(在EX_MEM)
        else if (forwardB == 1 || forwardB ==3){
            if(EX_MEM.ins.opcode == "add") tmp_rt = tmp_WB; 
            else tmp_rt = - tmp_WB;
        }

        tmp_MEM = tmp_rs + tmp_rt;
        
    }else if(EX_MEM.ins.opcode == "lw" || EX_MEM.ins.opcode == "sw"){
        EX_MEM.ins.immediate = registers[EX_MEM.ins.rs] + (EX_MEM.ins.immediate>>2); // immediate = rs + (immediate>>2
        if(EX_MEM.ins.opcode == "sw"){
            if(EX_MEM.controlSignals[5]==1 && EX_MEM.ins.rd && EX_MEM.ins.rd == ID_EX.ins.rt){
                tmp_MEM=tmp_EX;
            }
        }
    }

    // 處理 beq (在 EX 階段真正判斷是否跳)
    if (EX_MEM.ins.opcode == "beq") {
        // 若 rs == rt，branch taken
        if (registers[EX_MEM.ins.rs] == registers[EX_MEM.ins.rt]) {
            // PC += immediate
            PC --; // 這裡是beq PC+1的地方
            PC += EX_MEM.ins.immediate; // 修正這裡，直接加上 immediate

            // Flush：清除未來 pipeline 階段中「已經抓到但還沒執行完」的指令
            IF_ID.valid = false;
            ID_EX.valid = false;
        }
        // 若 rs != rt，則 branch not taken，什麼都不做
    }

    
    cout << EX_MEM.ins.opcode << ": EX" << endl;
}

void MEM() {
    if(!EX_MEM.valid) return; // 如果EX_MEM沒有東西就不做

    if(EX_MEM.ins.opcode == "lw") {
        tmp_WB = memory[EX_MEM.ins.immediate];
    } else if(EX_MEM.ins.opcode == "sw") {
        if(MEM_WB.controlSignals[5]==1 && MEM_WB.ins.rd && MEM_WB.ins.rd == EX_MEM.ins.rt){
            memory[EX_MEM.ins.immediate] = tmp_MEM;
        }
    }else{
        tmp_WB = tmp_MEM;
    }

    MEM_WB = EX_MEM; // 把EX_MEM的指令傳到MEM_WB
    MEM_WB.ins.immediate = EX_MEM.ins.immediate; // 把EX_MEM的immediate傳到MEM_WB
    MEM_WB.valid = true; // MEM_WB在MEM之後才會有指令
    EX_MEM.valid = false; // 用完了


    cout << MEM_WB.ins.opcode << ": MEM" << endl;
}

void WB() {
    if(!MEM_WB.valid) return; // 如果MEM_WB沒有指令，則跳過
    // 只有這三個有WB
    if(MEM_WB.ins.opcode == "add" || MEM_WB.ins.opcode == "sub") {
        registers[MEM_WB.ins.rd] = tmp_MEM;
    }
    if(MEM_WB.ins.opcode == "lw") {
        registers[MEM_WB.ins.rt] = tmp_WB;
    }
    cout << MEM_WB.ins.opcode << ": WB" << endl;
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

void simulate() {
    while(true) {
        if(stall) stall--;
        // 每次迴圈代表一個cycle
        cout << "\nCycle " << cycle << endl;
        WB();
        MEM();
        EX();
        ID();
        IF();
        if(!IF_ID.valid && !ID_EX.valid && !EX_MEM.valid && !MEM_WB.valid && PC >= instructionMemory.size()) break; // 如果全部都沒有指令了就結束
        cycle++;
    }
    //打印暫存器值
    cout << "Simulation complete. Register values:" << endl;
    for (int i = 0; i < 32; ++i) {
        cout << "$" << i << ": " << registers[i] << endl;
    }
    //打印記憶體值
    cout << "Memory values:" << endl;
    for (int i = 0; i < 32; ++i) {
        cout << "w[" << i << "]: " << memory[i] << endl;
    }
}


int main() {
    init();
    readInput("../inputs/test3.txt");
    simulate();
    return 0;
}