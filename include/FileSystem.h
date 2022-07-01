#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

class Buf;
class SuperBlock;
class Inode;
class DiskDriver;
class BufferManager;

/*
 * 文件系统存储资源管理块(Super Block)的定义。
 */
class SuperBlock
{
public:
	const static int MAX_NFREE = 100;  //  最大空闲存储资源管理块数量
	const static int MAX_NINODE = 100; //  最大Inode数量
									   /* Functions */
public:
	/* Constructors */
	SuperBlock();
	/* Destructors */
	~SuperBlock();
	//  格式化文件系统存储资源管理块
	void Format();

	/* Members */
public:
	int s_isize; /* 外存Inode区占用的盘块数 */
	int s_fsize; /* 盘块总数 */

	int s_nfree;	 /* 直接管理的空闲盘块数量 */
	int s_free[100]; /* 直接管理的空闲盘块索引表 */

	int s_ninode;	  /* 直接管理的空闲外存Inode数量 */
	int s_inode[100]; /* 直接管理的空闲外存Inode索引表 */

	int s_flock; /* 封锁空闲盘块索引表标志 */
	int s_ilock; /* 封锁空闲Inode表标志 */

	int s_fmod;		 /* 内存中super block副本被修改标志，意味着需要更新外存对应的Super Block */
	int s_ronly;	 /* 本文件系统只能读出 */
	int s_time;		 /* 最近一次更新时间 */
	int padding[47]; /* 填充使SuperBlock块大小等于1024字节，占据2个扇区 */
};

/*
 * 文件系统类(FileSystem)管理文件存储设备中
 * 的各类存储资源，磁盘块、外存INode的分配、
 * 释放。
 */
class FileSystem
{
public:
	/* static consts */
	static const int SUPER_BLOCK_SECTOR_NUMBER = 200; /* 定义SuperBlock位于磁盘上的扇区号，占据200，201两个扇区。 */
	static const int ROOTINO = 0; /* 文件系统根目录外存Inode编号 */
	static const int INODE_NUMBER_PER_SECTOR = 8; /* 外存INode对象长度为64字节，每个磁盘块可以存放512/64 = 8个外存Inode */
	static const int INODE_ZONE_START_SECTOR = 2; /* 外存Inode区位于磁盘上的起始扇区号 */
	static const int INODE_ZONE_SIZE = 1024 - 2;  /* 磁盘上外存Inode区占据的扇区数 */
	static const int INODE_NUM = INODE_NUMBER_PER_SECTOR * INODE_ZONE_SIZE;
	static const int DISK_SIZE = 16384; //  磁盘所有扇区数量
	static const int DATA_ZONE_START_SECTOR = 1024;						  /* 数据区的起始扇区号 */
	static const int DATA_ZONE_END_SECTOR = DISK_SIZE - 1;				  // 18000 - 1;	/* 数据区的结束扇区号 */
	static const int DATA_ZONE_SIZE = DISK_SIZE - DATA_ZONE_START_SECTOR; /* 数据区占据的扇区数量 */
	// Block块大小
	static const int BLOCK_SIZE = 512;
	// 定义SuperBlock位于磁盘上的扇区号，占据两个扇区
	static const int SUPERBLOCK_START_SECTOR = 0;
public:
	FileSystem();
	~FileSystem();
	//  格式化文件系统
	void FormatFS();
	/*初始化成员变量*/
	void Initialize();
	/*系统初始化时读入SuperBlock*/
	void LoadSuperBlock();
	/*根据文件存储设备获取该文件系统的SuperBlock*/
	SuperBlock *GetFS();
	/*将SuperBlock对象的内存副本更新到存储设备的SuperBlock中去*/
	void Update();
	/*在存储设备上分配一个空闲外存INode，一般用于创建新的文件。*/
	Inode *IAlloc();
	/*释放存储设备上编号为number的外存INode，一般用于删除文件。*/
	void IFree(int number);
	/*在存储设备上分配空闲磁盘块*/
	Buf *Alloc();
	/*释放存储设备编号为blkno的磁盘块*/
	void Free(int blkno);
public:
	DiskDriver *diskDriver;
	SuperBlock *superBlock;
	BufferManager *bufferManager;
};

#endif
