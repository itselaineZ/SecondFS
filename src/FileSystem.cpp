#include "../include/FileSystem.h"
#include "../include/Kernel.h"
#include "../include/Utility.h"
#include <string.h>
//#include "OpenFileManager.cpp"
SuperBlock g_spb;
extern DiskDriver d_DiskDriver;
extern BufferManager g_BufferManager;
extern InodeTable g_InodeTable;
extern User g_User;
/*==============================class SuperBlock===================================*/

SuperBlock::SuperBlock()
{
	// nothing to do here
}

SuperBlock::~SuperBlock()
{
	// nothing to do here
}

void SuperBlock::Format()
{
	s_isize = FileSystem::INODE_ZONE_SIZE;
	s_fsize = FileSystem::DISK_SIZE;
	s_nfree = 0;
	s_free[0] = -1;
	s_ninode = 0;
	s_flock = 0;
	s_ilock = 0;
	s_fmod = 0;
	s_ronly = 0;
	time((time_t *)&s_time);
}
/*==============================class FileSystem===================================*/
FileSystem::FileSystem()
{
	diskDriver = &d_DiskDriver;
	superBlock = &g_spb;
	bufferManager = &g_BufferManager;
	if (!diskDriver->Exists())
		FormatFS();
	else
		LoadSuperBlock();
}

FileSystem::~FileSystem()
{
	// nothing to do here
}

//  格式化文件系统
void FileSystem::FormatFS()
{
	superBlock->Format();
	diskDriver->Initialize();

	//空文件写入superBlock占据空间(不设置大小)
	diskDriver->write(superBlock, sizeof(SuperBlock), 0);
	DiskInode emptyDiskInode, rootDiskInode;
	//  根目录DiskNode
	rootDiskInode.d_mode |= Inode::IALLOC | Inode::IFDIR;	  //  文件被使用、文件类型为目录文件
	rootDiskInode.d_nlink = 1;								  //  该文件在目录树中不同路径名的数量为1
	diskDriver->write(&rootDiskInode, sizeof(rootDiskInode)); //  磁盘写入根目录Inode信息

	//  从第1个DiskInode开始初始化，第0个固定用于"/"根目录，不允许改变
	for (int i = 1; i < FileSystem::INODE_NUM; ++i)
	{
		if (superBlock->s_ninode < SuperBlock::MAX_NINODE)			//  当前空闲外存inode数量没有超过定义的最大值
			superBlock->s_inode[superBlock->s_ninode++] = i;		//  向空闲外存索引表中填写inode编号
		diskDriver->write(&emptyDiskInode, sizeof(emptyDiskInode)); //  磁盘写入空Inode占位置
	}

	//  制作两个空闲盘块，并清空
	char freeBlock1[DiskDriver::BLOCK_SIZE], freeBlock2[DiskDriver::BLOCK_SIZE];
	memset(freeBlock1, 0, DiskDriver::BLOCK_SIZE);
	memset(freeBlock2, 0, DiskDriver::BLOCK_SIZE);

	for (int i = 0; i < FileSystem::DATA_ZONE_SIZE; ++i)
	{
		if (superBlock->s_nfree >= SuperBlock::MAX_NFREE)
		{
			//  把s_nfree和s_free（空闲盘块数和对应的盘块索引表）一起拷贝给freeblock2
			Utility::MemCopy(freeBlock2, &(superBlock->s_nfree), sizeof(int) + sizeof(superBlock->s_free));
			//  写入磁盘
			diskDriver->write(&freeBlock2, DiskDriver::BLOCK_SIZE);
			superBlock->s_nfree = 0;
		}
		else
			//  空盘块写入磁盘占位置
			diskDriver->write(freeBlock1, DiskDriver::BLOCK_SIZE);
		superBlock->s_free[superBlock->s_nfree++] = i + DATA_ZONE_START_SECTOR;
	}
	time((time_t *)&superBlock->s_time);
	//  再次写入SuperBlock（内容已经更新完毕）
	diskDriver->write(superBlock, sizeof(SuperBlock), 0);
}

void FileSystem::Initialize()
{
	// this->m_BufferManager = &g_BufferManager; 
	// this->updlock = 0;
}

void FileSystem::LoadSuperBlock()
{
	diskDriver->read(superBlock, sizeof(SuperBlock), SUPERBLOCK_START_SECTOR*BLOCK_SIZE);
}

SuperBlock *FileSystem::GetFS()
{
	return this->superBlock;
}

void FileSystem::Update()
{
	Buf* pBuf;
	superBlock->s_fmod = 0;
    superBlock->s_time = (int)Utility::time(NULL);
	for (int j = 0; j < 2; j++) {
		int* p = (int *)superBlock + j * 128;
		pBuf = this->bufferManager->GetBlk(FileSystem::SUPERBLOCK_START_SECTOR + j);
		Utility::MemCopy(pBuf->b_addr, p, BLOCK_SIZE);
		this->bufferManager->Bwrite(pBuf);
	}
	g_InodeTable.UpdateInodeTable();
	this->bufferManager->Bflush();
}

Inode *FileSystem::IAlloc()
{
	SuperBlock *sb = superBlock;
	Buf *pBuf;
	Inode *pNode;
	User &u = g_User;
	int ino; /* 分配到的空闲外存Inode编号 */

	/*
	 * SuperBlock直接管理的空闲Inode索引表已空，
	 * 必须到磁盘上搜索空闲Inode。先对inode列表上锁，
	 * 因为在以下程序中会进行读盘操作可能会导致进程切换，
	 * 其他进程有可能访问该索引表，将会导致不一致性。
	 */
	if (sb->s_ninode <= 0)
	{
		/* 外存Inode编号从0开始，这不同于Unix V6中外存Inode从1开始编号 */
		ino = -1;

		/* 依次读入磁盘Inode区中的磁盘块，搜索其中空闲外存Inode，记入空闲Inode索引表 */
		for (int i = 0; i < sb->s_isize; i++)
		{
			pBuf = this->bufferManager->Bread(FileSystem::INODE_ZONE_START_SECTOR + i);

			/* 获取缓冲区首址 */
			int *p = (int *)pBuf->b_addr;

			/* 检查该缓冲区中每个外存Inode的i_mode != 0，表示已经被占用 */
			for (int j = 0; j < FileSystem::INODE_NUMBER_PER_SECTOR; j++)
			{
				ino++;

				int mode = *(p + j * sizeof(DiskInode) / sizeof(int));

				/* 该外存Inode已被占用，不能记入空闲Inode索引表 */
				if (mode != 0)
				{
					continue;
				}

				/*
				 * 如果外存inode的i_mode==0，此时并不能确定
				 * 该inode是空闲的，因为有可能是内存inode没有写到
				 * 磁盘上,所以要继续搜索内存inode中是否有相应的项
				 */
				if (g_InodeTable.IsLoaded(ino) == -1)
				{
					/* 该外存Inode没有对应的内存拷贝，将其记入空闲Inode索引表 */
					sb->s_inode[sb->s_ninode++] = ino;

					/* 如果空闲索引表已经装满，则不继续搜索 */
					if (sb->s_ninode >= 100)
					{
						break;
					}
				}
			}

			/* 至此已读完当前磁盘块，释放相应的缓存 */
			this->bufferManager->Brelse(pBuf);

			/* 如果空闲索引表已经装满，则不继续搜索 */
			if (sb->s_ninode >= 100)
			{
				break;
			}
		}

		/* 如果在磁盘上没有搜索到任何可用外存Inode，返回NULL */
		if (sb->s_ninode <= 0)
		{
			cout << "No Space On %d !\n";
			u.u_error = User::U_ENOSPC;
			return NULL;
		}
	}

	/*
	 * 上面部分已经保证，除非系统中没有可用外存Inode，
	 * 否则空闲Inode索引表中必定会记录可用外存Inode的编号。
	 */
	/* 从索引表“栈顶”获取空闲外存Inode编号 */
	ino = sb->s_inode[--sb->s_ninode];

	/* 将空闲Inode读入内存 */
	pNode = g_InodeTable.IGet(ino);
	/* 未能分配到内存inode */
	if (NULL == pNode)
	{
		cout << "No Space On %d !\n";
		return NULL;
	}
	pNode->Clean();
	/* 设置SuperBlock被修改标志 */
	sb->s_fmod = 1;
	return pNode; /* GCC likes it! */
}

/* 释放编号为number的外存INode，一般用于删除文件。*/
void FileSystem::IFree(int number) {
	if (superBlock->s_ninode >= SuperBlock::MAX_NINODE) {
		return ;
	}
	superBlock->s_inode[superBlock->s_ninode++] = number;
	superBlock->s_fmod = 1;
}

Buf *FileSystem::Alloc()
{
	int blkno; /* 分配到的空闲磁盘块编号 */
	Buf *pBuf;
	User &u = g_User;

	/* 从索引表“栈顶”获取空闲磁盘块编号 */
	blkno = superBlock->s_free[--superBlock->s_nfree];

	/*
	 * 若获取磁盘块编号为零，则表示已分配尽所有的空闲磁盘块。
	 * 或者分配到的空闲磁盘块编号不属于数据盘块区域中(由BadBlock()检查)，
	 * 都意味着分配空闲磁盘块操作失败。
	 */
	if (blkno <= 0)
	{
		superBlock->s_nfree = 0;
		u.u_error = User::U_ENOSPC;
		return NULL;
	}

	/*
	 * 栈已空，新分配到空闲磁盘块中记录了下一组空闲磁盘块的编号,
	 * 将下一组空闲磁盘块的编号读入SuperBlock的空闲磁盘块索引表s_free[100]中。
	 */
	if (superBlock->s_nfree <= 0)
	{

		/* 读入该空闲磁盘块 */
		pBuf = this->bufferManager->Bread(blkno);

		/* 从该磁盘块的0字节开始记录，共占据4(s_nfree)+400(s_free[100])个字节 */
		int *p = (int *)pBuf->b_addr;

		/* 首先读出空闲盘块数s_nfree */
		superBlock->s_nfree = *p++;

		/* 读取缓存中后续位置的数据，写入到SuperBlock空闲盘块索引表s_free[100]中 */
		Utility::MemCopy(superBlock->s_free, p, sizeof(superBlock->s_free));

		/* 缓存使用完毕，释放以便被其它进程使用 */
		this->bufferManager->Brelse(pBuf);
	}

	/* 普通情况下成功分配到一空闲磁盘块 */
	pBuf = this->bufferManager->GetBlk(blkno); /* 为该磁盘块申请缓存 */
	if (pBuf)
		this->bufferManager->ClrBuf(pBuf); /* 清空缓存中的数据 */
	superBlock->s_fmod = 1;					 /* 设置SuperBlock被修改标志 */

	return pBuf;
}

/* 释放存储设备dev上编号为blkno的磁盘块 */
void FileSystem::Free(int blkno) {
	Buf* pBuffer;
	User& u = g_User;

	if (superBlock->s_nfree >=SuperBlock::MAX_NFREE ) {
		pBuffer = this->bufferManager->GetBlk(blkno);
		int *p = (int*)pBuffer->b_addr;
		*p++ = superBlock->s_nfree;
		Utility::MemCopy(p, superBlock->s_free, sizeof(int)*SuperBlock::MAX_NFREE);
		superBlock->s_nfree = 0;
		this->bufferManager->Bwrite(pBuffer);
	}

	superBlock->s_free[superBlock->s_nfree++] = blkno;
	superBlock->s_fmod = 1;
}