# 計算機組織期末專題

## 系統需求
- **作業系統**：Windows
- **依賴工具**：C++

## 環境設置
### 1. 所需軟體與工具
請按照下列步驟進行安裝：
- 安裝 Microsoft Visual Studio (推薦)，或其他支援 C++ 開發環境的 IDE。
- 下載 MinGW (如需)，確保已安裝 g++ 編譯器。

## 執行指令
### 在專案跟目錄下終端機輸入以下指令產生exe檔，filename可以更換名稱
- g++ -o src/filename src/main.cpp
### 產生exe檔之後請輸入以下指令
- src\filename.exe

## 範例執行
- g++ -o src/main src/main.cpp
- src\main.exe

## 備註
換測資請改main.cpp裡的readInput的檔名，若要新增隱藏測資請放在inputs的資料夾

## 開發環境設定
以 Visual Studio Code 為例，使用下列設定檔：

```json
{
    "configurations": [
        {
            "name": "Win32",
            "includePath": [
                "${workspaceFolder}/**"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE"
            ],
            "windowsSdkVersion": "10.0.22621.0",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "gcc-x64",
            "compilerPath": "C:/msys64/mingw64/bin/g++.exe"
        }
    ],
    "version": 4
}
```
