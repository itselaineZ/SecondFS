#pragma
#include "../include/Utility.h"

void Utility::MemSet(void *s, int ch, size_t n) {
	::memset(s, ch, n);
}

void Utility::MemCopy(void *des, const void* src, unsigned int count)
{
	unsigned char* psrc = (unsigned char*)src;
	unsigned char* pdes = (unsigned char*)des;
	
	for ( unsigned int i = 0; i < count; i++ ) 
		pdes[i] = psrc[i];
}

int Utility::MemCmp(const void *buf1, const void *buf2, unsigned int count) {
	return ::memcmp(buf1, buf2, count);
}

void Utility::StringCopy(char* src, char* dst)
{
	while ( (*dst++ = *src++) != 0 ) ;
}

int Utility::StringLength(char* pString)
{
	int length = 0;
	char* pChar = pString;

	while( *pChar++ )
	{
		length++;
	}

	/* 返回字符串长度 */
	return length;
}

void Utility::DWordCopy(int *src, int *dst, int count)
{
	while(count--)
	{
		*dst++ = *src++;
	}
	return;
}

int Utility::Min(int a, int b)
{
	if(a < b)
		return a;
	return b;
}

void Utility::IOMove(unsigned char* from, unsigned char* to, int count)
{
	while(count--)
	{
		*to++ = *from++;
	}
	return;
}

time_t Utility::time(time_t* t) {
	return ::time(t);
}
