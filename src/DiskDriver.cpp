#include <iostream>
#include "DiskDriver.h"

const char* DiskDriver::DISK_FILE_NAME = "1952339-SecondFS.img";

DiskDriver::DiskDriver(){
    fp = fopen(DISK_FILE_NAME, "rb+");
}

DiskDriver::~DiskDriver(){
    if (fp){
        fclose(fp);
    }
}

void DiskDriver::Initialize(){
    fp = fopen(DISK_FILE_NAME, "wb+");
    if (fp == NULL){
        printf("Open File %s Error!\n", DISK_FILE_NAME);
        exit(-1);
    }
}

/*
void DiskDriver::IO(Buf* bp){
    if (bp->b_flags & Buf::B_WRITE){ //  写操作
        if (offset >= 0) {
            fseek(fp, offset, origin);
        }
        fread(buffer, size, 1, fp);
    }
    else if (bp->b_flasg & Buf::B_READ){    //  读操作

    }
}*/

void DiskDriver::read(void* buffer, unsigned int size, int offset, unsigned int origin) {
    if (offset >= 0)
        fseek(fp, offset, origin);
    fread(buffer, size, 1, fp);
}

void DiskDriver::write(const void* buffer, unsigned int size, int offset, unsigned int origin) {
    if (offset >= 0) 
        fseek(fp, offset, origin);
    fwrite(buffer, size, 1, fp);
}