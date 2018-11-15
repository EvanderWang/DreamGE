Cross Platform:

1. Windows: VSCode + cmake + ninja + clang-cl + vs link tool + vs debug tool

init setting: 
    first of all: need vs installed so that could use the link and debug tool

    1) Add Environment Path: {VSDir}/Common7/Tools ------- so that could find "VsDevCmd.bat"
    2) Add Environment Path: {VSDir}/VC            ------- so that could find "vcvarsall.bat"
    3) Set VSCode User Setting: "terminal.integrated.shell.windows": "cmd.exe"
    4) Set VSCode User Setting: "terminal.integrated.shellArgs.windows": ["/c","VsDevCmd.bat","&&","vcvarsall.bat","x64","&&","echo",]

2. MacOS: VSCode + cmake + ninja + clang + llvm link tool + lldb

VSCode Extensions:

1.C/C++
2.C/C++ Clang Command Adapter
3.CMake
4.ninja-build
5.Tasks
6.vscode-icons