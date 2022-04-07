#include <iostream>
#include <cstdio>
#include <string>
#include <sstream>
#include <vector>
#include "../include/File.h"
#include "../include/FileManager.h"
#include "../include/FileSystem.h"
#include "../include/OpenFileManager.h"
#include "../include/User.h"
#include "../include/Kernel.h"
using namespace std;

extern OpenFileTable g_OpenFileTable;
extern InodeTable g_INodeTable;
extern BufferManager g_BufferManager;
extern FileSystem g_FileSystem;

User g_User;

void Help() {
    static string man = 
        "Fformat                            : 格式化文件系统\n"
        "exit                               : 退出文件系统\n"
        "mkdir <dir>                        : 新建目录\n"
        "cd <dir>                           : 改变目录\n"
        "ls                                 : 列出目录及文件\n"
        "create <file> [-r -w]              : 新建文件\n"
        "delete <file>                      : 删除文件\n"
        "open <file> [-r -w]                : 打开文件\n"
        "close <file>                       : 关闭文件\n"
        "seek <file> <offset> <origin>      : 移动读写指针\n"
        "write <file1> <file2> <size>       : 将file2中内容写入file1，指定写入size字节\n"
        "read <file1> [-o <file2>] <size>   : 将file1内容输出到file2，默认输出到shell\n"
        "autoTest                           : 使用自动测试\n"
        ;
    cout << man;
}

void InitSystem() {
    cout << "Initializing System....\n";
    FileManager *fileManager = &Kernel::Instance().GetFileManager();
    fileManager->rootDirInode = g_InodeTable.IGet(FileSystem::ROOTINO);
}

int main() {

    InitSystem();

    User* user = &g_User;
    vector<string>arg;
    string cmd;

    cout << "Welcome to Elaine's Second File System! Input \"help\" to get the usage.\n";
    while(1) {
        cout << "[root@SecondFS " << user->u_curdir << " ]#";
        string line,cmd;
        getline(cin,line);
        stringstream ssin(line);
        ssin>>cmd;
        arg.push_back(cmd);
        int n=1;
        while (ssin >> arg[n]){ 
            n++;
        }

        if (arg[0] == "")
            continue;
        else if (arg[0] == "help") {
            Help();
        }
        else if (arg[0] == "Fformat") {
            g_OpenFileTable.Format();
            g_INodeTable.Format();
            g_BufferManager.FormatBuffer();
            g_FileSystem.FormatFS();
            cout << "Format Successfully, will be shut down automatically.\n";
            exit(0);
        }
        else if (arg[0] == "exit") {
            exit(0);
        }
        else if (arg[0] == "autoTest") {
            //autoTest();
            cout << "autoTest finished successfully\n";
        }
        else if (arg[0] == "mkdir") {
            user->Mkdir(arg[1]);
        }
        else if (arg[0] == "ls") {
            user->Ls();
        }
        else if (arg[0] == "cd") {
            user->Cd(arg[1]);
        }
        else if (arg[0] == "create") {
            user->Create(arg[1], arg[2]+arg[3]);
        }
        else if (arg[0] == "delete") {
            user->Delete(arg[1]);
        }
        else if (arg[0] == "open") {
            user->Open(arg[1], arg[2]);
        }
        else if (arg[0] == "close") {
            user->Close(arg[1]);
        }
        else if (arg[0] == "seek") {
            user->Seek(arg[1], arg[2], arg[3]);
        }
        else if (arg[0] == "read") {
            if (arg[1] == "-o")
                user->Read(arg[1], arg[3], arg[4]);
            else
                user->Read(arg[1], "", arg[2]);
        }
        else if (arg[0] == "write") {
            user->Write(arg[1], arg[2], arg[3]);
        }
        else if (arg[0] != "") {
            cout << "command error!\n";
        }
    }
}