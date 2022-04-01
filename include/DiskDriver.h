#ifndef DISKDRIVER_H
#define DISKDRIVER_H

#include "Buf.h"

class DiskDriver {
public:
    DiskDriver();
    ~DiskDriver();
    void Initialize();  //  初始化磁盘镜像
    //void IO(Buf* bp);  //  根据缓存控制块读写
    void read(void* buffer, unsigned int size, int offset, unsigned int origin);   //  读文件
    void write(const void* buffer, unsigned int size, int offset, unsigned int origin);       //  写文件

private:
    static const char *DISK_FILE_NAME;  //  磁盘镜像文件名
    static const int BLOCK_SIZE = 512;  //  数据块大小为512字节
    FILE *fp;   //  磁盘镜像文件指针
};

#endif