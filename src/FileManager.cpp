#include "../include/FileManager.h"
#include "../include/Kernel.h"
#include "../include/Inode.h"
#include "../include/Utility.h"

extern BufferManager g_BufferManager;
extern User g_User;
extern InodeTable g_InodeTable;
extern FileSystem g_FileSystem;

/*==========================class FileManager===============================*/
FileManager::FileManager()
{
	m_FileSystem = &g_FileSystem;
    m_OpenFileTable = &g_OpenFileTable;
    m_InodeTable = &g_InodeTable;
    rootDirInode = m_InodeTable->IGet(FileSystem::ROOTINO);
	//rootDirInode->i_count = 0;
    rootDirInode->i_count += 0xff;
}

FileManager::~FileManager()
{
	// nothing to do here
}

void FileManager::Initialize()
{
	this->m_FileSystem = &g_FileSystem;

	this->m_InodeTable = &g_InodeTable;
	this->m_OpenFileTable = &g_OpenFileTable;

	this->m_InodeTable->Initialize();
}

/*
 * 功能：打开文件
 * 效果：建立打开文件结构，内存i节点开锁 、i_count 为正数（i_count ++）
 * */
void FileManager::Open()
{
	Inode *pInode;
	User &u = g_User;

	pInode = this->NameI(FileManager::OPEN); /* 0 = Open, not create */
	/* 没有找到相应的Inode */
	if (NULL == pInode)
	{
		return;
	}
	this->Open1(pInode, u.u_arg[1], 0);
}

/*
 * 功能：创建一个新的文件
 * 效果：建立打开文件结构，内存i节点开锁 、i_count 为正数（应该是 1）
 * */
void FileManager::Creat()
{
	Inode *pInode;
	User &u = g_User;
	//unsigned int newACCMode = u.u_arg[1] & (Inode::IRWXU | Inode::IRWXG | Inode::IRWXO);
	unsigned int newACCMode = u.u_arg[1];

	/* 搜索目录的模式为1，表示创建；若父目录不可写，出错返回 */
	pInode = this->NameI(FileManager::CREATE);
	/* 没有找到相应的Inode，或NameI出错 */
	if (NULL == pInode)
	{
		if (u.u_error)
			return;
		/* 创建Inode */
		pInode = this->MakNode(newACCMode & (~Inode::ISVTX));
		/* 创建失败 */
		if (NULL == pInode)
		{
			return;
		}

		/*
		 * 如果所希望的名字不存在，使用参数trf = 2来调用open1()。
		 * 不需要进行权限检查，因为刚刚建立的文件的权限和传入参数mode
		 * 所表示的权限内容是一样的。
		 */
		this->Open1(pInode, File::FWRITE, 2);
		//pInode->i_mode |= 
	}
	else
	{
		/* 如果NameI()搜索到已经存在要创建的文件，则清空该文件（用算法ITrunc()）。UID没有改变
		 * 原来UNIX的设计是这样：文件看上去就像新建的文件一样。然而，新文件所有者和许可权方式没变。
		 * 也就是说creat指定的RWX比特无效。
		 * 邓蓉认为这是不合理的，应该改变。
		 * 现在的实现：creat指定的RWX比特有效 */
		this->Open1(pInode, File::FWRITE, 1);
		pInode->i_mode |= newACCMode;
	}
}

/*
 * trf == 0由open调用
 * trf == 1由creat调用，creat文件的时候搜索到同文件名的文件
 * trf == 2由creat调用，creat文件的时候未搜索到同文件名的文件，这是文件创建时更一般的情况
 * mode参数：打开文件模式，表示文件操作是 读、写还是读写
 */
void FileManager::Open1(Inode *pInode, int mode, int trf)
{
	User &u = g_User;

	/* 在creat文件的时候搜索到同文件名的文件，释放该文件所占据的所有盘块 */
	if (1 == trf)
	{
		pInode->ITrunc();
	}

	/* 分配打开文件控制块File结构 */
	File *pFile = this->m_OpenFileTable->FAlloc();
	if (NULL == pFile)
	{
		this->m_InodeTable->IPut(pInode);
		return;
	}
	/* 设置打开文件方式，建立File结构和内存Inode的勾连关系 */
	pFile->f_flag = mode & (File::FREAD | File::FWRITE);
	pFile->f_inode = pInode;

	/* 为打开或者创建文件的各种资源都已成功分配，函数返回 */
	if (u.u_error == 0)
	{
		return;
	}
	else /* 如果出错则释放资源 */
	{
		/* 释放打开文件描述符 */
		int fd = u.u_ar0[User::EAX];
		if (fd != -1)
		{
			u.u_ofiles.SetF(fd, NULL);
			/* 递减File结构和Inode的引用计数 ,File结构没有锁 f_count为0就是释放File结构了*/
			pFile->f_count--;
		}
		this->m_InodeTable->IPut(pInode);
	}
}

void FileManager::Close()
{
	User &u = g_User;
	int fd = u.u_arg[0];

	/* 获取打开文件控制块File结构 */
	File *pFile = u.u_ofiles.GetF(fd);
	if (NULL == pFile)
	{
		return;
	}

	/* 释放打开文件描述符fd，递减File结构引用计数 */
	u.u_ofiles.SetF(fd, NULL);
	this->m_OpenFileTable->CloseF(pFile);
}

void FileManager::Seek()
{
	File *pFile;
	User &u = g_User;
	int fd = u.u_arg[0];

	pFile = u.u_ofiles.GetF(fd);
	if (NULL == pFile)
	{
		return; /* 若FILE不存在，GetF有设出错码 */
	}

	/* 管道文件不允许seek */
	if (pFile->f_flag & File::FPIPE)
	{
		u.u_error = User::U_ESPIPE;
		return;
	}

	int offset = u.u_arg[1];

	/* 如果u.u_arg[2]在3 ~ 5之间，那么长度单位由字节变为512字节 */
	if (u.u_arg[2] > 2)
	{
		offset = offset << 9;
		u.u_arg[2] -= 3;
	}

	switch (u.u_arg[2])
	{
	/* 读写位置设置为offset */
	case 0:
		pFile->f_offset = offset;
		break;
	/* 读写位置加offset(可正可负) */
	case 1:
		pFile->f_offset += offset;
		break;
	/* 读写位置调整为文件长度加offset */
	case 2:
		pFile->f_offset = pFile->f_inode->i_size + offset;
		break;
	}
}

void FileManager::Read()
{
	/* 直接调用Rdwr()函数即可 */
	this->Rdwr(File::FREAD);
}

void FileManager::Write()
{
	/* 直接调用Rdwr()函数即可 */
	this->Rdwr(File::FWRITE);
}

void FileManager::Rdwr(enum File::FileFlags mode)
{
	File *pFile;
	User &u = g_User;

	/* 根据Read()/Write()的系统调用参数fd获取打开文件控制块结构 */
	pFile = u.u_ofiles.GetF(u.u_arg[0]); /* fd */
	if (NULL == pFile)
	{
		/* 不存在该打开文件，GetF已经设置过出错码，所以这里不需要再设置了 */
		/*	u.u_error = User::EBADF;	*/
		return;
	}

	/* 读写的模式不正确 */
	if ((pFile->f_flag & mode) == 0)
	{
		u.u_error = User::U_EACCES;
		return;
	}
	u.u_IOParam.m_Base = (unsigned char*)(u.u_arg[1]); /* 目标缓冲区首址 */
	u.u_IOParam.m_Count = u.u_arg[2];				  /* 要求读/写的字节数 */

	u.u_IOParam.m_Offset = pFile->f_offset;
	if (File::FREAD == mode)
	{
		pFile->f_inode->ReadI();
	}
	else
	{
		pFile->f_inode->WriteI();
	}

	/* 根据读写字数，移动文件读写偏移指针 */
	pFile->f_offset += (u.u_arg[2] - u.u_IOParam.m_Count);

	/* 返回实际读写的字节数，修改存放系统调用返回值的核心栈单元 */
	u.u_ar0[User::EAX] = u.u_arg[2] - u.u_IOParam.m_Count;
}

/* 返回NULL表示目录搜索失败，否则是根指针，指向文件的内存打开i节点 ，上锁的内存i节点  */
Inode* FileManager::NameI(enum DirectorySearchMode mode) {
    //if (NULL == rootDirINode) {
    //    rootDirINode = inodeTable->IGet(FileSystem::ROOT_INODE_NO);
    //    //rootDirINode多加一次，始终存在以免被IPut
    //    rootDirINode->i_count++;
    //    //open file table ?
    //    return rootDirINode;
    //}

    BufferManager& bufferManager = g_BufferManager;
    User &u = g_User;
    Inode* pINode = u.u_cdir;
    Buf *pBuf;
    int freeEntryOffset;
    unsigned int index = 0, nindex = 0;
	Utility uti;


    if ('/' == u.u_dirp[0]) {
        nindex = ++index + 1;
        pINode = this->rootDirInode;
    }

    /* 如果试图更改和删除当前目录文件则出错 */
    /*if ('\0' == curchar && mode != FileManager::OPEN)
    {
        u.u_error = User::ENOENT;
        goto out;
    }*/

    /* 外层循环每次处理pathname中一段路径分量 */
    while (1) {
        if (u.u_error != User::U_NOERROR) {
            break;
        }
        if (nindex >= u.u_dirp.length()) {
            return pINode;
        }

        /* 如果要进行搜索的不是目录文件，释放相关INode资源则退出 */
        if ((pINode->i_mode&Inode::IFMT) != Inode::IFDIR) {
            u.u_error = User::U_ENOTDIR;
            break;
        }

        nindex = u.u_dirp.find_first_of('/', index);
        Utility::MemSet(u.u_dbuf, 0, sizeof(u.u_dbuf));
        Utility::MemCopy(u.u_dbuf, u.u_dirp.data() + index, (nindex == (unsigned int)string::npos ? u.u_dirp.length() : nindex) - index);
        index = nindex + 1;

        /* 内层循环部分对于u.dbuf[]中的路径名分量，逐个搜寻匹配的目录项 */
        u.u_IOParam.m_Offset = 0;
        /* 设置为目录项个数 ，含空白的目录项*/
        u.u_IOParam.m_Count = pINode->i_size / sizeof(DirectoryEntry);
        freeEntryOffset = 0;
        pBuf = NULL;

        /* 在一个目录下寻找 */
        while (1) {

            /* 对目录项已经搜索完毕 */
            if (0 == u.u_IOParam.m_Count) {
                if (NULL != pBuf) {
                    bufferManager.Brelse(pBuf);
                }

                /* 如果是创建新文件 */
                if (FileManager::CREATE == mode && nindex >= u.u_dirp.length()) {
                    u.u_pdir = pINode;
                    if (freeEntryOffset) {
                        u.u_IOParam.m_Offset = freeEntryOffset - sizeof(DirectoryEntry);
                    }
                    else {
                        pINode->i_flag |= Inode::IUPD;
                    }
                    return NULL;
                }
                u.u_error = User::U_ENOENT;
                goto out;
            }

            /* 已读完目录文件的当前盘块，需要读入下一目录项数据盘块 */
            if (0 == u.u_IOParam.m_Offset%Inode::BLOCK_SIZE) {
                if (pBuf) {
                    bufferManager.Brelse(pBuf);
                }
                int phyBlkno = pINode->Bmap(u.u_IOParam.m_Offset / Inode::BLOCK_SIZE);
                pBuf = bufferManager.Bread(phyBlkno);
                //pBuffer->debug();
            }

            Utility::MemCopy(&u.u_dent, pBuf->b_addr + (u.u_IOParam.m_Offset % Inode::BLOCK_SIZE), sizeof(u.u_dent));
            u.u_IOParam.m_Offset += sizeof(DirectoryEntry);
            u.u_IOParam.m_Count--;

            /* 如果是空闲目录项，记录该项位于目录文件中偏移量 */
            if (0 == u.u_dent.m_ino) {
                if (0 == freeEntryOffset) {
                    freeEntryOffset = u.u_IOParam.m_Offset;
                }
                continue;
            }

            if (!(uti.MemCmp(u.u_dbuf, &u.u_dent.m_name, sizeof(DirectoryEntry) - 4))) {
                break;
            }
        }

        if (pBuf) {
            bufferManager.Brelse(pBuf);
        }

        /* 如果是删除操作，则返回父目录INode，而要删除文件的INode号在u.dent.m_ino中 */
        if (FileManager::DELETE == mode && nindex >= u.u_dirp.length()) {
            return pINode;
        }

        /*
        * 匹配目录项成功，则释放当前目录INode，根据匹配成功的
        * 目录项m_ino字段获取相应下一级目录或文件的INode。
        */
        this->m_InodeTable->IPut(pINode);
        pINode = this->m_InodeTable->IGet(u.u_dent.m_ino);

        if (NULL == pINode) {
            return NULL;
        }
    }

out:
    this->m_InodeTable->IPut(pINode);
    return NULL;
}

// char FileManager::NextChar()
// {
// 	User &u = g_User;

// 	/* u.u_dirp指向pathname中的字符 */
// 	return *u.u_dirp++;
// }

/* 由creat调用。
 * 为新创建的文件写新的i节点和新的目录项
 * 返回的pInode是上了锁的内存i节点，其中的i_count是 1。
 *
 * 在程序的最后会调用 WriteDir，在这里把属于自己的目录项写进父目录，修改父目录文件的i节点 、将其写回磁盘。
 *
 */
Inode *FileManager::MakNode(unsigned int mode)
{
	Inode *pInode;
	User &u = g_User;

	/* 分配一个空闲DiskInode，里面内容已全部清空 */
	pInode = this->m_FileSystem->IAlloc();
	if (NULL == pInode)
	{
		return NULL;
	}

	pInode->i_flag |= (Inode::IACC | Inode::IUPD);
	pInode->i_mode = mode | Inode::IALLOC;
	pInode->i_nlink = 1;
	//多用户要在此处改动
	/* 将目录项写入u.u_dent，随后写入目录文件 */
	this->WriteDir(pInode);
	return pInode;
}

void FileManager::WriteDir(Inode *pInode)
{
	User &u = g_User;

	/* 设置目录项中Inode编号部分 */
	u.u_dent.m_ino = pInode->i_number;

	/* 设置目录项中pathname分量部分 */
	for (int i = 0; i < DirectoryEntry::DIRSIZ; i++)
	{
		u.u_dent.m_name[i] = u.u_dbuf[i];
	}

	u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
	u.u_IOParam.m_Base = (unsigned char *)&u.u_dent;

	/* 将目录项写入父目录文件 */
	u.u_pdir->WriteI();
	this->m_InodeTable->IPut(u.u_pdir);
}

/*
 * 返回值是0，表示拥有打开文件的权限；1表示没有所需的访问权限。文件未能打开的原因记录在u.u_error变量中。
 */
int FileManager::Access(Inode *pInode, unsigned int mode)
{
	User &u = g_User;

	/* 对于写的权限，必须检查该文件系统是否是只读的 */
	if (Inode::IWRITE == mode)
	{
		if (this->m_FileSystem->GetFS()->s_ronly != 0)
		{
			u.u_error = User::U_EROFS;
			return 1;
		}
	}
	/*
	 * 对于超级用户，读写任何文件都是允许的
	 * 而要执行某文件时，必须在i_mode有可执行标志
	 */
	// if ( u.u_uid == 0 )
	// {
	// 	if ( Inode::IEXEC == mode && ( pInode->i_mode & (Inode::IEXEC | (Inode::IEXEC >> 3) | (Inode::IEXEC >> 6)) ) == 0 )
	// 	{
	// 		u.u_error = User::U_EACCES;
	// 		return 1;
	// 	}
	// 	return 0;	/* Permission Check Succeed! */
	// }

	// if ( u.u_uid != pInode->i_uid )
	// {
	// 	mode = mode >> 3;
	// 	if ( u.u_gid != pInode->i_gid )
	// 	{
	// 		mode = mode >> 3;
	// 	}
	// }
	if ((pInode->i_mode & mode) != 0)
	{
		return 0;
	}

	u.u_error = User::U_EACCES;
	return 1;
}

void FileManager::ChDir()
{
	Inode *pInode;
	User &u = g_User;

	pInode = this->NameI(FileManager::OPEN);
	if (NULL == pInode)
	{
		return;
	}
	/* 搜索到的文件不是目录文件 */
	if ((pInode->i_mode & Inode::IFMT) != Inode::IFDIR)
	{
		u.u_error = User::U_ENOTDIR;
		this->m_InodeTable->IPut(pInode);
		return;
	}
	u.u_cdir = pInode;
	/* 路径不是从根目录'/'开始，则在现有u.u_curdir后面加上当前路径分量 */
	if (u.u_dirp[0] != '/')
	{
		u.u_curdir += u.u_dirp;
	}
	else
	{
		/* 如果是从根目录'/'开始，则取代原有工作目录 */
		u.u_curdir = u.u_dirp;
	}
	if (u.u_curdir.back() != '/')
		u.u_curdir.push_back('/');

	// this->SetCurDir((char *)u.u_arg[0] /* pathname */);
}

void FileManager::UnLink()
{
	Inode *pInode;
	Inode *pDeleteInode;
	User &u = g_User;

	pDeleteInode = this->NameI(FileManager::DELETE);
	if (NULL == pDeleteInode)
	{
		return;
	}

	pInode = this->m_InodeTable->IGet(u.u_dent.m_ino);
	if (NULL == pInode)
	{
		return;
	}

	/* 写入清零后的目录项 */
	u.u_IOParam.m_Offset -= (DirectoryEntry::DIRSIZ + 4);
	u.u_IOParam.m_Base = (unsigned char *)&u.u_dent;
	u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;

	u.u_dent.m_ino = 0;
	pDeleteInode->WriteI();

	/* 修改inode项 */
	pInode->i_nlink--;
	pInode->i_flag |= Inode::IUPD;

	this->m_InodeTable->IPut(pDeleteInode);
	this->m_InodeTable->IPut(pInode);
}

//  列出当前Inode节点的文件项
void FileManager::Ls() {
    User &u = g_User;
    BufferManager& bufferManager = g_BufferManager;

	Inode *pInode = u.u_cdir;
	Buf *pBuf = NULL;
	u.u_IOParam.m_Offset = 0; //  当前文件读写偏移量
	//  当前还剩余的读、写字节数量，设置为当前Inode节点文件大小/目录项大小
	u.u_IOParam.m_Count = pInode->i_size / sizeof(DirectoryEntry);
	while (u.u_IOParam.m_Count)
	{
		if (u.u_IOParam.m_Offset % Inode::BLOCK_SIZE == 0)
		{
			if (pBuf)
				bufferManager.Brelse(pBuf); //  释放
			//  计算逻辑块号
			int phyBlkno = pInode->Bmap(u.u_IOParam.m_Offset / Inode::BLOCK_SIZE);
			pBuf = bufferManager.Bread(phyBlkno);
		}
		//  将当前Inode节点的文件项，拷贝到u_udent（当前目录的目录项)结构中存储
		Utility::MemCopy(&u.u_dent, pBuf->b_addr + (u.u_IOParam.m_Offset % Inode::BLOCK_SIZE), sizeof(u.u_dent));
		//  IO位置后移
		u.u_IOParam.m_Offset += sizeof(DirectoryEntry);
		u.u_IOParam.m_Count--;
		//
		if (!u.u_dent.m_ino) //  无目录项
			continue;
		u.u_ls += u.u_dent.m_name;
		u.u_ls += '\n';
	}
	if (pBuf)
		bufferManager.Brelse(pBuf);
}
/*==========================class DirectoryEntry===============================*/
DirectoryEntry::DirectoryEntry()
{
	this->m_ino = 0;
	this->m_name[0] = '\0';
}

DirectoryEntry::~DirectoryEntry()
{
	// nothing to do here
}
