#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include <bits/stdc++.h>
#include <unordered_map>
#include "Buf.h"
using namespace std;

class DiskDriver;
//class Kernel;

class BufferManager
{
public:
	/* static const member */
	static const int NBUF = 100;			/* 缓存控制块、缓冲区的数量 */
	static const int BUFFER_SIZE = 512;	/* 缓冲区大小。 以字节为单位 */

public:
	BufferManager();
	~BufferManager();

	void FormatBuffer();				//  格式化缓存控制块
	
	void Initialize();					/* 缓存控制块队列的初始化。将缓存控制块中b_addr指向相应缓冲区首地址。*/
	
	Buf* GetBlk(int blkno);	/* 申请一块缓存，用于读写设备dev上的字符块blkno。*/
	void Brelse(Buf* bp);				/* 释放缓存控制块buf */
	//void IOWait(Buf* bp);				/* 同步方式I/O，等待I/O操作结束 */
	//void IODone(Buf* bp);				/* I/O操作结束善后处理 */

	Buf* Bread(int blkno);	/* 读一个磁盘块。dev为主、次设备号，blkno为目标磁盘块逻辑块号。 */

	void Bwrite(Buf* bp);				/* 写一个磁盘块 */
	void Bdwrite(Buf* bp);				/* 延迟写磁盘块 */

	void ClrBuf(Buf* bp);				/* 清空缓冲区内容 */
	void Bflush();				/* 将dev指定设备队列中延迟写的缓存全部输出到磁盘 */
	
    //Buf& GetBFreeList();				/* 获取自由缓存队列控制块Buf对象引用 */
	void DetachNode(Buf *bp);	//  LRU Cache算法，每次从头部取出，使用后放到尾部

private:
	//void GetError(Buf* bp);				/* 获取I/O操作中发生的错误信息 */ //#
	void NotAvail(Buf* bp);				/* 从自由队列中摘下指定的缓存控制块buf */ //#
	//Buf* InCore(int blkno);	/* 检查指定字符块是否已在缓存中 */ //#
	
private:
	Buf* bFreeList;						/* 自由缓存队列控制块 */
	Buf SwBuf;							/* 进程图像传送请求块 */
	Buf m_Buf[NBUF];					/* 缓存控制块数组 */
	unsigned char Buffer[NBUF][BUFFER_SIZE];	/* 缓冲区数组 */
	map<int, Buf*> mp;
	DiskDriver* m_DiskDriver;		/* 指向设备管理模块全局对象 */
};

#endif
