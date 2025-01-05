
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
    cout<<"\nFinal Result:"<<endl;
    cout<<"\nTotal Cycles: "<<cycle<<endl<<endl;
    //打印暫存器值
    cout << "Final Register Values:" << endl;
    for (int i = 0; i < 32; ++i) {
        cout << registers[i] << " ";