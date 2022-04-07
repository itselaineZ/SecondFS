#include "../include/Kernel.h"

Kernel Kernel::instance;

/*
 * 设备管理、高速缓存管理全局manager
 */
BufferManager g_BufferManager;
DiskDriver g_DiskDriver;

/*
 * 文件系统相关全局manager
 */
FileSystem g_FileSystem;
FileManager g_FileManager;

Kernel::Kernel()
{
}

Kernel::~Kernel()
{
}

Kernel& Kernel::Instance()
{
	return Kernel::instance;
}

void Kernel::InitBuffer()
{
	this->m_BufferManager = &g_BufferManager;
	this->m_DiskDriver = &g_DiskDriver;

	cout<<"Initialize Buffer...";
	this->GetBufferManager().Initialize();
	cout<<"OK.\n";

	cout<<"Initialize Device Manager...";
	this->GetDiskDriver().Initialize();
	cout<<"OK.\n";
}

void Kernel::InitFileSystem()
{
	this->m_FileSystem = &g_FileSystem;
	this->m_FileManager = &g_FileManager;

	cout<<"Initialize File System...";
	this->GetFileSystem().Initialize();
	cout<<"OK.\n";

	cout<<"Initialize File Manager...";
	this->GetFileManager().Initialize();
	cout<<"OK.\n";
}

void Kernel::Initialize()
{
	// InitMemory();
	// InitProcess();
	InitBuffer();
	InitFileSystem();
}

BufferManager& Kernel::GetBufferManager()
{
	return *(this->m_BufferManager);
}

DiskDriver& Kernel::GetDiskDriver()
{
	return *(this->m_DiskDriver);
}

FileSystem& Kernel::GetFileSystem()
{
	return *(this->m_FileSystem);
}

FileManager& Kernel::GetFileManager()
{
	return *(this->m_FileManager);
}

User& Kernel::GetUser()
{
	return *(User*)USER_ADDRESS;
}
