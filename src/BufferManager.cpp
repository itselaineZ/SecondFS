#include "BufferManager.h"
#include "Kernel.h"

BufferManager::BufferManager()
{
	//nothing to do here
}

BufferManager::~BufferManager()
{
	//nothing to do here
}

void BufferManager::Initialize()
{
	int i;
	Buf* bp;

	this->bFreeList.b_forw = this->bFreeList.b_back = &(this->bFreeList);
	this->bFreeList.av_forw = this->bFreeList.av_back = &(this->bFreeList);

	for(i = 0; i < NBUF; i++)
	{
		bp = &(this->m_Buf[i]);
		bp->b_dev = -1;
		bp->b_addr = this->Buffer[i];
		/* 初始化NODEV队列 */
		bp->b_back = &(this->bFreeList);
		bp->b_forw = this->bFreeList.b_forw;
		this->bFreeList.b_forw->b_back = bp;
		this->bFreeList.b_forw = bp;
		/* 初始化自由队列 */
		bp->b_flags = Buf::B_BUSY;
		Brelse(bp);
	}
	this->m_DeviceManager = &Kernel::Instance().GetDeviceManager();
	return;
}

Buf* BufferManager::GetBlk(short dev, int blkno)
{
	Buf* bp;
	Devtab* dp;
	User& u = Kernel::Instance().GetUser();

	/* 如果主设备号超出了系统中块设备数量 */
	if( Utility::GetMajor(dev) >= this->m_DeviceManager->GetNBlkDev() )
	{
		Utility::Panic("nblkdev: There doesn't exist the device");
	}

	/* 
	 * 如果设备队列中已经存在相应缓存，则返回该缓存；
	 * 否则从自由队列中分配新的缓存用于字符块读写。
	 */
loop:
	/* 表示请求NODEV设备中字符块 */
	if(dev < 0)
	{
		dp = (Devtab *)(&this->bFreeList);
	}
	else
	{
		short major = Utility::GetMajor(dev);
		/* 根据主设备号获得块设备表 */
		dp = this->m_DeviceManager->GetBlockDevice(major).d_tab;

		if(dp == NULL)
		{
			Utility::Panic("Null devtab!");
		}
		/* 首先在该设备队列中搜索是否有相应的缓存 */
		for(bp = dp->b_forw; bp != (Buf *)dp; bp = bp->b_forw)
		{
			/* 不是要找的缓存，则继续 */
			if(bp->b_blkno != blkno || bp->b_dev != dev)
				continue;

			/* 
			 * 临界区之所以要从这里开始，而不是从上面的for循环开始。
			 * 主要是因为，中断服务程序并不会去修改块设备表中的
			 * 设备buf队列(b_forw)，所以不会引起冲突。
			 */
			X86Assembly::CLI();
			if(bp->b_flags & Buf::B_BUSY)
			{
				bp->b_flags |= Buf::B_WANTED;
				u.u_procp->Sleep((unsigned long)bp, ProcessManager::PRIBIO);
				X86Assembly::STI();
				goto loop;
			}
			X86Assembly::STI();
			/* 从自由队列中抽取出来 */
			this->NotAvail(bp);
			return bp;
		}
	}//end of else

	X86Assembly::CLI();
	/* 如果自由队列为空 */
	if(this->bFreeList.av_forw == &this->bFreeList)
	{
		this->bFreeList.b_flags |= Buf::B_WANTED;
		u.u_procp->Sleep((unsigned long)&this->bFreeList, ProcessManager::PRIBIO);
		X86Assembly::STI();
		goto loop;
	}
	X86Assembly::STI();

	/* 取自由队列第一个空闲块 */
	bp = this->bFreeList.av_forw;
	this->NotAvail(bp);

	/* 如果该字符块是延迟写，将其异步写到磁盘上 */
	if(bp->b_flags & Buf::B_DELWRI)
	{
		bp->b_flags |= Buf::B_ASYNC;
		this->Bwrite(bp);
		goto loop;
	}
	/* 注意: 这里清除了所有其他位，只设了B_BUSY */
	bp->b_flags = Buf::B_BUSY;

	/* 从原设备队列中抽出 */
	bp->b_back->b_forw = bp->b_forw;
	bp->b_forw->b_back = bp->b_back;
	/* 加入新的设备队列 */
	bp->b_forw = dp->b_forw;
	bp->b_back = (Buf *)dp;
	dp->b_forw->b_back = bp;
	dp->b_forw = bp;

	bp->b_dev = dev;
	bp->b_blkno = blkno;
	return bp;
}

void BufferManager::Brelse(Buf* bp)
{
	ProcessManager& procMgr = Kernel::Instance().GetProcessManager();

	if(bp->b_flags & Buf::B_WANTED)
	{
		procMgr.WakeUpAll((unsigned long)bp);
	}

	/* 如果有进程正在等待分配自由队列中的缓存，则唤醒相应进程 */
	if(this->bFreeList.b_flags & Buf::B_WANTED)
	{
		/* 清楚B_WANTED标志，表示已有空闲缓存 */
		this->bFreeList.b_flags &= (~Buf::B_WANTED);
		procMgr.WakeUpAll((unsigned long)&this->bFreeList);
	}

	/* 如果有错误发生，则将次设备号改掉，
	 * 避免后续进程误用缓冲区中的错误数据。
	 */
	if(bp->b_flags & Buf::B_ERROR)
	{
		Utility::SetMinor(bp->b_dev, -1);
	}

	/* 临界资源，比如：在同步读末期会调用这个函数，
	 * 此时很有可能会产生磁盘中断，同样会调用这个函数。
	 */
	X86Assembly::CLI();		/* spl6();  UNIX V6的做法 */

	/* 注意以下操作并没有清除B_DELWRI、B_WRITE、B_READ、B_DONE标志
	 * B_DELWRI表示虽然将该控制块释放到自由队列里面，但是有可能还没有些到磁盘上。
	 * B_DONE则是指该缓存的内容正确地反映了存储在或应存储在磁盘上的信息 
	 */
	bp->b_flags &= ~(Buf::B_WANTED | Buf::B_BUSY | Buf::B_ASYNC);
	(this->bFreeList.av_back)->av_forw = bp;
	bp->av_back = this->bFreeList.av_back;
	bp->av_forw = &(this->bFreeList);
	this->bFreeList.av_back = bp;
	
	X86Assembly::STI();
	return;
}

void BufferManager::IOWait(Buf* bp)
{
	User& u = Kernel::Instance().GetUser();

	/* 这里涉及到临界区
	 * 因为在执行这段程序的时候，很有可能出现硬盘中断，
	 * 在硬盘中断中，将会修改B_DONE如果此时已经进入循环
	 * 则将使得改进程永远睡眠
	 */
	X86Assembly::CLI();
	while( (bp->b_flags & Buf::B_DONE) == 0 )
	{
		u.u_procp->Sleep((unsigned long)bp, ProcessManager::PRIBIO);
	}
	X86Assembly::STI();

	this->GetError(bp);
	return;
}

void BufferManager::IODone(Buf* bp)
{
	/* 置上I/O完成标志 */
	bp->b_flags |= Buf::B_DONE;
	if(bp->b_flags & Buf::B_ASYNC)
	{
		/* 如果是异步操作,立即释放缓存块 */
		this->Brelse(bp);
	}
	else
	{
		/* 清除B_WANTED标志位 */
		bp->b_flags &= (~Buf::B_WANTED);
		Kernel::Instance().GetProcessManager().WakeUpAll((unsigned long)bp);
	}
	return;
}

Buf* BufferManager::Bread(short dev, int blkno)
{
	Buf* bp;
	/* 根据设备号，字符块号申请缓存 */
	bp = this->GetBlk(dev, blkno);
	/* 如果在设备队列中找到所需缓存，即B_DONE已设置，就不需进行I/O操作 */
	if(bp->b_flags & Buf::B_DONE)
	{
		return bp;
	}
	/* 没有找到相应缓存，构成I/O读请求块 */
	bp->b_flags |= Buf::B_READ;
	bp->b_wcount = BufferManager::BUFFER_SIZE;

	/* 
	 * 将I/O请求块送入相应设备I/O请求队列，如无其它I/O请求，则将立即执行本次I/O请求；
	 * 否则等待当前I/O请求执行完毕后，由中断处理程序启动执行此请求。
	 * 注：Strategy()函数将I/O请求块送入设备请求队列后，不等I/O操作执行完毕，就直接返回。
	 */
	this->m_DeviceManager->GetBlockDevice(Utility::GetMajor(dev)).Strategy(bp);
	/* 同步读，等待I/O操作结束 */
	this->IOWait(bp);
	return bp;
}


void BufferManager::Bwrite(Buf *bp)
{
	unsigned int flags;

	flags = bp->b_flags;
	bp->b_flags &= ~(Buf::B_READ | Buf::B_DONE | Buf::B_ERROR | Buf::B_DELWRI);
	bp->b_wcount = BufferManager::BUFFER_SIZE;		/* 512字节 */

	this->m_DeviceManager->GetBlockDevice(Utility::GetMajor(bp->b_dev)).Strategy(bp);

	if( (flags & Buf::B_ASYNC) == 0 )
	{
		/* 同步写，需要等待I/O操作结束 */
		this->IOWait(bp);
		this->Brelse(bp);
	}
	else if( (flags & Buf::B_DELWRI) == 0)
	{
	/* 
	 * 如果不是延迟写，则检查错误；否则不检查。
	 * 这是因为如果延迟写，则很有可能当前进程不是
	 * 操作这一缓存块的进程，而在GetError()主要是
	 * 给当前进程附上错误标志。
	 */
		this->GetError(bp);
	}
	return;
}

void BufferManager::Bdwrite(Buf *bp)
{
	/* 置上B_DONE允许其它进程使用该磁盘块内容 */
	bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
	this->Brelse(bp);
	return;
}

void BufferManager::ClrBuf(Buf *bp)
{
	int* pInt = (int *)bp->b_addr;

	/* 将缓冲区中数据清零 */
	for(unsigned int i = 0; i < BufferManager::BUFFER_SIZE / sizeof(int); i++)
	{
		pInt[i] = 0;
	}
	return;
}

void BufferManager::Bflush(short dev)
{
	Buf* bp;
	/* 注意：这里之所以要在搜索到一个块之后重新开始搜索，
	 * 因为在bwite()进入到驱动程序中时有开中断的操作，所以
	 * 等到bwrite执行完成后，CPU已处于开中断状态，所以很
	 * 有可能在这期间产生磁盘中断，使得bfreelist队列出现变化，
	 * 如果这里继续往下搜索，而不是重新开始搜索那么很可能在
	 * 操作bfreelist队列的时候出现错误。
	 */
loop:
	X86Assembly::CLI();
	for(bp = this->bFreeList.av_forw; bp != &(this->bFreeList); bp = bp->av_forw)
	{
		/* 找出自由队列中所有延迟写的块 */
		if( (bp->b_flags & Buf::B_DELWRI) && (dev == DeviceManager::NODEV || dev == bp->b_dev) )
		{
			bp->b_flags |= Buf::B_ASYNC;
			this->NotAvail(bp);
			this->Bwrite(bp);
			goto loop;
		}
	}
	X86Assembly::STI();
	return;
}



void BufferManager::GetError(Buf* bp)
{
	User& u = Kernel::Instance().GetUser();

	if (bp->b_flags & Buf::B_ERROR)
	{
		u.u_error = User::EIO;
	}
	return;
}

void BufferManager::NotAvail(Buf *bp)
{
	X86Assembly::CLI();		/* spl6();  UNIX V6的做法 */
	/* 从自由队列中取出 */
	bp->av_back->av_forw = bp->av_forw;
	bp->av_forw->av_back = bp->av_back;
	/* 设置B_BUSY标志 */
	bp->b_flags |= Buf::B_BUSY;
	X86Assembly::STI();
	return;
}

Buf* BufferManager::InCore(short adev, int blkno)
{
	Buf* bp;
	Devtab* dp;
	short major = Utility::GetMajor(adev);

	dp = this->m_DeviceManager->GetBlockDevice(major).d_tab;
	for(bp = dp->b_forw; bp != (Buf *)dp; bp = bp->b_forw)
	{
		if(bp->b_blkno == blkno && bp->b_dev == adev)
			return bp;
	}
	return NULL;
}

Buf& BufferManager::GetBFreeList()
{
	return this->bFreeList;
}
