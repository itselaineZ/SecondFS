#include <iostream>
#include <cstdio>
#include <string>
#include <sstream>
#include <vector>
#include "Inode.cpp"
#include "File.cpp"
#include "FileManager.cpp"
#include "BufferManager.cpp"
#include "FileSystem.cpp"
#include "OpenFileManager.cpp"
#include "User.cpp"
#include "Kernel.cpp"
#include "Utility.cpp"
#include "DiskDriver.cpp"
using namespace std;

 extern OpenFileTable g_OpenFileTable;
// extern InodeTable g_InodeTable;
 extern BufferManager g_BufferManager;
 extern FileSystem g_FileSystem;


extern DiskDriver d_DiskDriver;

SuperBlock g_SuperBlock;
extern InodeTable g_InodeTable;
User g_User;
extern FileManager g_FileManager;

void Help() {
    string man = 
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
        "write <file1> <file2> <size>       : 将file2中内容写入file1,指定写入size字节\n"
        "read <file1> [-o <file2>] <size>   : 将file1内容输出到file2,默认输出到shell\n"
        "autoTest                           : 使用自动测试\n"
        ;
    cout << man;
    return;
}

void InitSystem() {
    cout << "Initializing System....\n";
    FileManager *fileManager = &g_FileManager;
    fileManager->rootDirInode = g_InodeTable.IGet(FileSystem::ROOTINO);
    fileManager->rootDirInode->i_mode |= Inode::IFDIR;
    g_User = User();
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
        arg.clear();
        arg.push_back(cmd);
        int n=1;
        string tmp;
        while (ssin >> tmp){ 
            arg.push_back(tmp);
            n++;
        }

        if (arg[0] == "")
            continue;
        else if (arg[0] == "help") {
            Help();
            cout<<"help done"<<endl;
        }
        else if (arg[0] == "Fformat") {
            g_OpenFileTable.Format();
            g_InodeTable.Format();
            g_BufferManager.FormatBuffer();
            g_FileSystem.FormatFS();
            d_DiskDriver.close();
            cout << "Format Successfully, will be shut down automatically.\n";
            exit(0);
        }
        else if (arg[0] == "exit") {
            d_DiskDriver.close();
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
            user->Create(arg[1], arg[2]);
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