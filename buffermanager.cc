#include "buffermanager.h"
#include <map>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <list>
#include <cassert>
#define D if (1)
using namespace std;
unsigned char nullfill[BlockSize];
struct FileStatus {
	FILE *fp;
	int fileSize;
};
static map <string,FileStatus> dict;
map <string, list <int> >freelist;
struct Buffer {
	string fileName;
	int offset;
	bool dirty;
	unsigned char data[BlockSize];
};

static FileStatus open(const string &fileName) {
	if (dict.find(fileName)!=dict.end()) 
		return dict[fileName];
	FileStatus ret;
	FILE *fplist=fopen((fileName+".freelist").c_str(),"r");
	if (fplist) {
		int o;
		while (fscanf(fplist,"%d",&o)!=EOF) {
			freelist[fileName].push_back(o);
		}
		fclose(fplist);
	}

	if ( (ret.fp = fopen(fileName.c_str(),"rb+")) ==0 ) {
		ret.fp=fopen(fileName.c_str(),"w");
		fclose(ret.fp);
		ret.fp=fopen(fileName.c_str(),"rb+");		
		ret.fileSize=0;

	} else {
		fseek(ret.fp,0,SEEK_END);
		ret.fileSize=ftell(ret.fp);
	}
	assert(ret.fp);
	dict[fileName]=ret;		
	return ret;
}

Block bmNewBlock(const string &fileName) {	
	FileStatus fs=open(fileName);
	if (freelist[fileName].empty()) {
		fseek(fs.fp,0,SEEK_END);
		fwrite(nullfill,BlockSize,1,fs.fp);
		int offset=fs.fileSize;
		fs.fileSize+=BlockSize;
		dict[fileName]=fs;
		return bmReadBlock(fileName,offset);
	} else {

		int offset=freelist[fileName].front();
		fseek(fs.fp,offset,SEEK_SET);
		fwrite(nullfill,BlockSize,1,fs.fp);
		freelist[fileName].pop_front();
		return bmReadBlock(fileName,offset);		
	}
}
list <Buffer> bufferlist;
void writeBack(const Buffer &buffer) {
	if (buffer.dirty) {
		FileStatus fs=open(buffer.fileName);
		fseek(fs.fp,buffer.offset,SEEK_SET);
		fwrite(buffer.data,BlockSize,1,fs.fp);	
	}
}
void readintobuffer(const string &fileName, int offset) {
	Buffer ret;
	ret.fileName=fileName;
	ret.offset=offset;
	ret.dirty=false;	
	if (bufferlist.size()==BufferSize) {
		writeBack(bufferlist.front());
		bufferlist.pop_front();		
	} 
	FileStatus fs=open(fileName);
	fseek(fs.fp,offset,SEEK_SET);
	fread(ret.data,BlockSize,1,fs.fp);
	Buffer g=ret;
	bufferlist.push_back(ret);
}
void seebuffer() {
	puts("---------");
	for (list<Buffer>:: iterator it=bufferlist.begin();it!=bufferlist.end();it++) {
		printf("offset=%d dirty=%d ",it->offset,it->dirty);
		for (int i=0;i<10;i++) printf("%d ",it->data[i]);
		printf("\n");
	}		
	puts("---------");	
}
Block bmReadBlock(const string &fileName, int offset) {
	Block ret;
	ret.fileName=fileName;
	ret.offset=offset;
	for (list<Buffer> ::iterator it=bufferlist.begin();it!=bufferlist.end();it++) {
		if (it->fileName==fileName && it->offset == offset) {
			memcpy(ret.data,it->data,BlockSize);

			Buffer tmp=*it;
			bufferlist.erase(it);
			bufferlist.push_back(tmp);
			return ret;
		} 
	}
	readintobuffer(fileName,offset);
	memcpy(ret.data,bufferlist.back().data,BlockSize);
	return ret;
}

void  bmWriteBlock(struct Block &b) {
	for (list<Buffer>::iterator it=bufferlist.begin();it!=bufferlist.end();it++) {
		if (it->fileName==b.fileName && it->offset==b.offset) {
			memcpy(it->data,b.data,BlockSize);
			it->dirty=true;						
			Buffer tmp=*it;
			bufferlist.erase(it);
			bufferlist.push_back(tmp);
			return;
		}
	}
	readintobuffer(b.fileName,b.offset);
	memcpy(bufferlist.back().data,b.data,BlockSize);
	bufferlist.back().dirty=true;
}
void flushBuffer() {

	for (list<Buffer>:: iterator it=bufferlist.begin();it!=bufferlist.end();it++) {
		writeBack(*it);
		it->dirty=false;
	}
	for (map <string, list <int> >::iterator it=freelist.begin();it!=freelist.end();it++) {
		FILE *fp=fopen((it->first+".freelist").c_str(),"w");
		assert(fp);
		for (list<int>::iterator lit=it->second.begin();lit!=it->second.end();lit++) {
			fprintf(fp,"%d\n",*lit);
		}
		fclose(fp);
	}
}
void  bmReleaseBlock(const string &fileName,int offset) {
	open(fileName);
	for (list<int>::iterator it=freelist[fileName].begin();it!=freelist[fileName].end();it++) {
		if (offset==*it)
			assert(false);
	}

	for (list<Buffer>::iterator it=bufferlist.begin();it!=bufferlist.end();it++) {
		if (it->fileName==fileName && it->offset == offset) {
			bufferlist.erase(it);
			break;
		}
	}

	freelist[fileName].push_back(offset);

}
void bmClear(const string &fileName) {
	if (dict.find(fileName)!=dict.end()) {
		FileStatus fs=open(fileName);
		fclose(fs.fp);
		dict.erase(dict.find(fileName));
	}
	if (freelist.find(fileName)!=freelist.end()) 
		freelist.erase(freelist.find(fileName));
	while (1) {
		bool deleted=false;
		for (list<Buffer>::iterator it=bufferlist.begin();it!=bufferlist.end();it++) {
			if (it->fileName==fileName) {
				bufferlist.erase(it);
				deleted=true;
				break;
			}
		}
		if (!deleted)
			break;
	}
	remove(fileName.c_str());
	remove((fileName+".freelist").c_str());	
}
class finallizer {
public:
	~finallizer() {
		flushBuffer();
	}
} fz;
