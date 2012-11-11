#ifndef _buffer_manager_h_
#define _buffer_manager_h_
#include <string>
#define BlockSize 100
#define BufferSize 100

using namespace std;

struct Block {
	unsigned char data[BlockSize];
	unsigned int offset;
	string fileName;
};
Block bmNewBlock(const string &fileName);
Block bmReadBlock(const string &fileName, int offset);
void  bmWriteBlock(struct Block &b);
void  flushBuffer();
void  bmReleaseBlock(const string &fileName, int offset);
void  seebuffer();
void  bmClear(const string &fileName);
#endif
