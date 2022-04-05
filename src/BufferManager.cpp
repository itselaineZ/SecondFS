#include "BufferManager.h"
#include "Kernel.h"
#include "Utility.h"

BufferManager::BufferManager()
{
	//nothing to do here
}

BufferManager::~BufferManager()
{
	//nothing to do here
}

void BufferManager::FormatBuffer() {
	Buf emptyBuffer;
    for (int i = 0; i < NBUF; ++i)
        Utility::MemCopy(m_Buf + i, &emptyBuffer, sizeof(Buf));
    Initialize();
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
	this->m_DiskDriver = &Kernel::Instance().GetDiskDriver();
	return;
}

void BufferManager::DetachNode(Buf *bp) {
	if (bp->b_back) {
		bp->b_forw->b_back = bp->b_back;
		bp->b_back->b_forw = bp->b_forw;
		bp->b_back = NULL;
		bp->b_forw = NULL;
	}
}

//  重写：申请一块缓存，从缓存队列中取出，用于读写设备上的块blkno
Buf* BufferManager::GetBlk(int blkno)
{
	Buf* bp;
	User& u = Kernel::Instance().GetUser();

	if (mp.find(blkno) != mp.end()) {
		bp = mp[blkno];
		DetachNode(bp);
		return bp;
	}

	bp = bFreeList->b_back;
	if (bp == bFreeList) {
		cout << "no Buf available!\n";
		return NULL;
	}
	DetachNode(bp);
	mp.erase(bp->b_blkno);
	if (bp->b_flags & Buf::B_DELWRI)
		m_DiskDriver->write(bp->b_addr, BUFFER_SIZE, bp->b_blkno*BUFFER_SIZE);
	bp->b_flags &= ~(Buf::B_DELWRI | Buf::B_DONE);
	bp->b_blkno = blkno;
	mp[blkno] = bp;
	return bp;
}

//  释放缓存控制块buf，将其添加到自由缓存队列控制块队首即可
void BufferManager::Brelse(Buf* bp) {
	if (bp->b_back != NULL) {
        return;
    }
    bp->b_forw = bFreeList->b_forw;
    bp->b_back = bFreeList;
    bFreeList->b_forw->b_back = bp;
    bFreeList->b_forw = bp;
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

//  重写：读一个磁盘块，blkno为目标磁盘块逻辑号
Buf* BufferManager::Bread(int blkno)
{
	Buf* bp;
	/* 根据设备号，字符块号申请缓存 */
	bp = this->GetBlk(blkno);
	/* 如果在设备队列中找到所需缓存，即B_DONE已设置，就不需进行I/O操作 */
	if(bp->b_flags & (Buf::B_DONE | Buf::B_DELWRI))
		return bp;
	m_DiskDriver->read(bp->b_addr, BUFFER_SIZE, bp->b_blkno*BUFFER_SIZE);
	bp->b_flags |= Buf::B_DONE;
	return bp;
}

//  写一个磁盘块
void BufferManager::Bwrite(Buf *bp)
{
	bp->b_flags &= ~(Buf::B_DELWRI);
	m_DiskDriver->write(bp->b_addr, BUFFER_SIZE, bp->b_blkno*BUFFER_SIZE);
	bp->b_flags |= (Buf::B_DONE);
	Brelse(bp);
}

void BufferManager::Bdwrite(Buf *bp)
{
	/* 置上B_DONE允许其它进程使用该磁盘块内容 */
	bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
	this->Brelse(bp);
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

