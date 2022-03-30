#include "../include/DiskDriver.h"

const char* DiskDriver::DISK_FILE_NAME = "1952339-张馨月.img"

void DiskDriver::DiskDriver(){
    fp = fopen(DISK_FILE_NAME, "rb+");
}

void DiskDriver::~DiskDriver(){
    if (fp){
        close(fp);
    }
}

void DiskDriver::Initialize(){
    fp = fopen(DISK_FILE_NAME, "wb+");
    if (fp == NULL){
        printf("Open File %s Error!\n", DISK_FILE_NAME);
        exit(-1);
    }
}

void DiskDriver::IO(Buf* bp){
    if (bp.b_flags & BUF::B_WRITE){ //  写操作

    }
    else if (bp.b_flasg & BUF::B_READ){

    }
}