#ifndef DISKDRIVER_H
#define DISKDRIVER_H

#include <iostream>
#include "Buf.h"
using namespace std;

class DiskDriver {
public:
    static const int BLOCK_SIZE = 512;  //  数据块大小为512字节
    
    DiskDriver();
    ~DiskDriver();
    void Initialize();  //  初始化磁盘镜像
    //void IO(Buf* bp);  //  根据缓存控制块读写
    bool Exists();
    void read(void* buffer, unsigned int size, int offset = -1, unsigned int origin = SEEK_SET);   //  读文件
    void write(const void* buffer, unsigned int size, int offset = -1, unsigned int origin = SEEK_SET);       //  写文件
    void close();
private:
    static const char *DISK_FILE_NAME;  //  磁盘镜像文件名
    FILE *fp;   //  磁盘镜像文件指针
};

#endif