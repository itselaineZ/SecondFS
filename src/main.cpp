#include <iostream>
#include <cstdio>
#include <cstring>
#include "File.h"
#include "FileManager.h"
#include "FileSystem.h"
#include "OpenFileManager.h"
#include "User.h"
using namespace std;

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
}

void InitSystem() {
    cout << "Initializing System....\n";
    FileManager *fileManager = &Kernel::Instance().GetFileManager();
    fileManager->rootDirInode = g_InodeTable.IGet(FileSystem::ROOTINO);
}

int main() {

    InitSystem();

    User* user = &g_User;
    string cmd;

    cout << "Welcome to Elaine's Second File System! Input \"help\" to get usage.\n";
    while(1) {
        cout << "[root@SecondFS " << user->u_curdir << " ]#";
        getline(cin, cmd);
        if (cmd == "")
            continue;
        else if (cmd == "help") {
            Help();
        }
        else if (cmd == "Fformat") {
            g_OpenFileTable.Format();
            g_INodeTable.Format();
            g_BufferManager.FormatBuffer();
            g_FileSystem.FormatFS();
        }
    }
}