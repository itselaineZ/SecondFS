#ifndef UITILITY_H
#define UITILITY_H

#include <ctime>

/*
 *@comment 定义一些工具常量
 * 由于使用了编译选项-fno-builtin，
 * 编译器不提供这些常量的定义。
 */
#define NULL	0

class Utility
{
public:
	static void MemCopy(unsigned long src, unsigned long des, unsigned int count);

	static void StringCopy(char* src, char* dst);

	static int StringLength(char* pString);

	/* 以src为源地址，dst为目的地址，复制count个双字 */
	static void DWordCopy(int* src, int* dst, int count);

	static int Min(int a, int b);

	/* 用于在读、写文件时，高速缓存与用户指定目标内存区域之间数据传送 */
	static void IOMove(unsigned char* from, unsigned char* to, int count);

	static time_t time(time_t* t);
};

#endif
