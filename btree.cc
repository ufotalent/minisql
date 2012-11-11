#include <string>
#include <cmath>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include <set>
#include "buffermanager.h"
#include "element.h"
#include "btree.h"
using namespace std;
struct BTreeHeader {
	int elementType;
	int stringLength;
	int rootOffset;
	int fanOut;
};

struct node {
	bool isRoot;	
	bool isLeaf;
	int p; //deserted!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	vector<int> sons;
	vector<element> data;
};

void nodeToBinary(const node &n, unsigned char b[BlockSize],BTreeHeader bth) {
	memset(b,0,BlockSize);
	b[0]=n.isRoot;
	b[1]=n.isLeaf;
	memcpy(&b[2],&n.p,sizeof(n.p));

	int numSons=n.sons.size();
	int numData=n.data.size();
	assert(numSons<=bth.fanOut);
	assert(numData<bth.fanOut);
	
	memcpy(&b[6],&numSons,sizeof(numSons));
	memcpy(&b[10],&numData,sizeof(numData));	
	int offset=14;

	for (int i=0;i<numSons;i++) {
		memcpy(&b[offset],&n.sons[i],sizeof(n.sons[i]));
		offset+=sizeof(n.sons[i]);
	}
	int null=0;
	for (int i=numSons;i<bth.fanOut;i++) {
		memcpy(&b[offset],&null,sizeof(null));
		offset+=sizeof(null);
	}	

	for (int i=0;i<numData;i++) {
		assert(n.data[i].type==bth.elementType); 
		switch (bth.elementType) {
		case 0:
			memcpy(&b[offset],&n.data[i].datai,sizeof(int) );
			offset+=sizeof(int);			
			break;
		case 1:
			memcpy(&b[offset],&n.data[i].dataf,sizeof(float) );
			offset+=sizeof(float);			
			break;
		case 2:
			assert(n.data[i].datas.length()<=bth.stringLength);
			memcpy(&b[offset],n.data[i].datas.c_str(),n.data[i].datas.length());
			offset+=bth.stringLength;
			break;
		default:
			assert(false);
		}
	}

}
int fetchint(const unsigned char *loc) {
	int ret;
	memcpy(&ret,loc,sizeof(int));
	return ret;
}
float fetchfloat(const unsigned char *loc) {
	float ret;
	memcpy(&ret,loc,sizeof(float));
	return ret;
}
string fetchstring(const unsigned char *loc,int len) {
	char ret[BlockSize];
	memcpy(ret,loc,len);
	ret[len]=0;
	return ret;
}
node binaryToNode(const unsigned char *b, BTreeHeader bth) {
	node ret;
	ret.isRoot=b[0];
	ret.isLeaf=b[1];


	ret.p=fetchint(b+2);
	int numSons=fetchint(b+6);
	int numData=fetchint(b+10);
	
	assert(numSons<=bth.fanOut);
	assert(numData<bth.fanOut);

	
	int offset=14;
	ret.sons.clear();
	for (int i=0;i<numSons;i++) {
		ret.sons.push_back(fetchint(b+offset));
		offset+=4;
	}
	offset=14+(bth.fanOut)*4;

	for (int i=0;i<numData;i++) {
		switch (bth.elementType) {
		case 0:
			ret.data.push_back(fetchint(b+offset));
			offset+=sizeof(int);
			break;
		case 1:
			ret.data.push_back(fetchfloat(b+offset));
			offset+=sizeof(float);			
			break;
		case 2:
			ret.data.push_back(fetchstring(b+offset,bth.stringLength));
			offset+=bth.stringLength;
			break;
		default:
			assert(false);
		}
	}
	return ret;
	
}
void btCreate(const string &fileName,int type, int length) { //int,float,string
	assert(type!=2 || length!=-1);

	BTreeHeader bth;
	bth.elementType=type;
	bth.stringLength=length;
	bth.rootOffset=BlockSize;
	
	switch(type) {
		case 0: bth.fanOut=(BlockSize-sizeof(bool)*2-sizeof(int)*3-sizeof(int))/(sizeof(int)+ sizeof(int)); break;
		case 1: bth.fanOut=(BlockSize-sizeof(bool)*2-sizeof(int)*3-sizeof(int))/(sizeof(int)+ sizeof(float));break;
		case 2: bth.fanOut=(BlockSize-sizeof(bool)*2-sizeof(int)*3-sizeof(int))/(sizeof(int)+ length);break;
		default: assert(false);
	}
	bth.fanOut++;
	Block b=bmNewBlock(fileName);
	assert(b.offset==0);
	memset(b.data,0,sizeof(b.data));
	memcpy(b.data,&bth,sizeof(BTreeHeader));
	bmWriteBlock(b);

	
	b=bmNewBlock(fileName);
	node Root;
	Root.isLeaf=true;
	Root.isRoot=true;
	Root.p=0;
	Root.sons.push_back(-1);

	nodeToBinary(Root,b.data,bth);	
	bmWriteBlock(b);
} 

node readNode(const string &fileName, int offset, const BTreeHeader &bth) {
	Block block=bmReadBlock(fileName,offset);		
	return binaryToNode(block.data,bth);
}

vector <int> searchpath;
int __find(const string &fileName,int offset, const element &key,const BTreeHeader &bth) {
	node now=readNode(fileName,offset,bth);
	if (now.isRoot) 
		searchpath.clear();
	searchpath.push_back(offset);
	if (now.isLeaf) 
		return offset;
	assert(now.sons.size()==now.data.size()+1);
	for (int i=0;i<now.data.size();i++) {
		if (now.data[i]>key)
			return __find(fileName,now.sons[i], key,bth);
	}
	return __find(fileName,now.sons[now.sons.size()-1],key,bth);
}

// find the offset of first element larger or equal to 
// return 0 if the element is larger then any record
int btFind(const string &fileName,const element &key) {

	BTreeHeader bth;
	Block block=bmReadBlock(fileName,0);
	memcpy(&bth,block.data,sizeof(bth));
	assert(bth.elementType==key.type);

	int offset=__find(fileName,bth.rootOffset,key,bth);
	node now=readNode(fileName,offset,bth);
	for (int i=0;i<now.data.size();i++) {
		if (now.data[i]>=key) {
			return now.sons[i];
		}
	}
	int next=*(now.sons.end()-1);
	if (next==-1) {
		return -1;
	}
	node nnode=readNode(fileName,next,bth);
	return nnode.sons[0];
}

void saveHeader(const BTreeHeader bth, const string fileName) {
	Block block=bmReadBlock(fileName,0);
	memset(block.data,0,BlockSize);
	memcpy(block.data,&bth,sizeof(bth));
	bmWriteBlock(block);
}
void insert_in_parent(Block BN,element keyp,Block BNP,const string &fileName,BTreeHeader bth) {
	node N=binaryToNode(BN.data,bth);
	node NP=binaryToNode(BNP.data,bth);
	if (N.isRoot) {
		Block newblock = bmNewBlock(fileName);
		node newnode;
		newnode.p=0;
		newnode.isRoot=true;
		newnode.isLeaf=false;
		newnode.sons.push_back(BN.offset);
		newnode.sons.push_back(BNP.offset);
		newnode.data.push_back(keyp);
		N.isRoot=false;


		bth.rootOffset=newblock.offset;
		saveHeader(bth,fileName);

		nodeToBinary(newnode,newblock.data,bth);
		bmWriteBlock(newblock);

		nodeToBinary(N,BN.data,bth);
		bmWriteBlock(BN);
	} else {
		int n=bth.fanOut;
		Block BP=bmReadBlock(fileName,*(searchpath.end()-2));
		node P = binaryToNode(BP.data,bth);
		assert(P.sons.size()==P.data.size()+1);
		if (P.sons.size()<n) {
			bool findd=false;
			for (int i=0;i<P.sons.size();i++) {
				if (P.sons[i]==BN.offset) {
					P.sons.insert(P.sons.begin()+i+1,BNP.offset);
					P.data.insert(P.data.begin()+i,keyp);
					nodeToBinary(P,BP.data,bth);
					bmWriteBlock(BP);
					findd=true;
					return;
				}
			}
			assert(findd);
		} else {
			node T=P;
			bool findd=false;

			for (int i=0;i<T.sons.size();i++) {
				if (T.sons[i]==BN.offset) {
					T.sons.insert(T.sons.begin()+i+1,BNP.offset);
					T.data.insert(T.data.begin()+i,keyp);
					findd=true;
					break;
				}
			}
			assert(findd);
			assert(T.sons.size()==n+1);

			P.sons.clear();
			P.data.clear();
			Block BPP=bmNewBlock(fileName);
			node PP;
			PP.isLeaf=false;
			PP.isRoot=false;
			PP.p=0;
		
			for (int i=0;i<(n+1)/2;i++) {
				P.sons.push_back(T.sons[i]);
				if (i!=(n+1)/2-1)
					P.data.push_back(T.data[i]);
			}
			element KPP=T.data[(n+1)/2-1];
			for (int i=(n+1)/2;i<=n;i++) {
				PP.sons.push_back(T.sons[i]);
				if (i!=n) 
					PP.data.push_back(T.data[i]);
			}
			nodeToBinary(P,BP.data,bth);
			nodeToBinary(PP,BPP.data,bth);
			bmWriteBlock(BP);
			bmWriteBlock(BPP);
			searchpath.pop_back();
			insert_in_parent(BP,KPP,BPP,fileName,bth);
		}
	}
}
void btInsert(const string &fileName,const element &key,int value) {
	BTreeHeader bth;
	Block block=bmReadBlock(fileName,0);
	memcpy(&bth,block.data,sizeof(bth));
	assert(bth.elementType==key.type);

	int offset=__find(fileName,bth.rootOffset,key,bth);
	block=bmReadBlock(fileName,offset);
	node now=binaryToNode(block.data,bth);	

	if (now.data.size()<bth.fanOut-1) {
		for (int i=0;i<now.data.size();i++) {
			if (now.data[i]>key) {
				now.data.insert(now.data.begin()+i,key);
				now.sons.insert(now.sons.begin()+i,value);
				nodeToBinary(now,block.data,bth);
				bmWriteBlock(block);
				return;
			}
		}
		now.data.push_back(key);
		now.sons.insert(now.sons.end()-1,value);
		nodeToBinary(now,block.data,bth);
		bmWriteBlock(block);		
		
	} else {
		Block newblock=bmNewBlock(fileName);
		node tmp=now;
		int n=bth.fanOut;		
		bool findd=false;
		for (int i=0;i<tmp.data.size();i++) {
			if (tmp.data[i]>key) {
				tmp.data.insert(tmp.data.begin()+i,key);
				tmp.sons.insert(tmp.sons.begin()+i,value);
				findd=true;
				break;
			}
		}
		if (!findd) {
			tmp.data.push_back(key);
			tmp.sons.insert(tmp.sons.end()-1,value);
		}



		node newnode;
		assert(now.sons.size()==n);
		int orgnext=now.sons[n-1];


		now.sons.clear();
		now.data.clear();
		for (int i=0;i<(n+1)/2;i++) {
			now.sons.push_back(tmp.sons[i]);
			now.data.push_back(tmp.data[i]);			
		}
		now.sons.push_back(newblock.offset);
		newnode.isLeaf=true;
		newnode.isRoot=false;
		newnode.p=0;
		for (int i=(n+1)/2;i<n;i++) {
			newnode.sons.push_back(tmp.sons[i]);
			newnode.data.push_back(tmp.data[i]);			
		}
		newnode.sons.push_back(orgnext);


		nodeToBinary(now,block.data,bth);
		bmWriteBlock(block);
		

		nodeToBinary(newnode,newblock.data,bth);
		bmWriteBlock(newblock);

		element keyp=newnode.data[0];
		insert_in_parent(block,keyp,newblock,fileName,bth);
	}
}

void seetree(string fileName,int offset,BTreeHeader bth) {
	node now=readNode(fileName,offset,bth);
	printf("ID=%d\n",offset);
	printf("pointers : ");
	for (int i=0;i<now.sons.size();i++) 
		printf("%d ",now.sons[i]);
	printf("\n");

	printf("data : ");
	for (int i=0;i<now.data.size();i++) 
		printf("%d ",now.data[i].datai);
	printf("\n");	
	if (now.isLeaf)
		return ;
	for (int i=0;i<now.sons.size();i++) 
		seetree(fileName,now.sons[i],bth);
}
void seetree(const string &filename) {
	Block block=bmReadBlock(filename,0);
	BTreeHeader bth;
	memcpy(&bth,block.data,sizeof(bth));
	printf("----------------\n");
	seetree(filename,bth.rootOffset,bth);
	printf("----------------\n");	
}

void delete_entry(Block BN, element K, int P, const string& fileName,BTreeHeader bth) {
	node N=binaryToNode(BN.data,bth);
	const int n=bth.fanOut;
	for (int i=0;i<N.data.size();i++) {
		if (N.data[i]==K) {
			N.data.erase(N.data.begin()+i);
			break;
		}
	}
	for (int i=0;i<N.sons.size();i++) {
		if (N.sons[i]==P) {
			N.sons.erase(N.sons.begin()+i);
			break;
		}
	}

	if (N.isRoot && N.sons.size()==1) { 

		if (N.sons[0]==-1) {
			nodeToBinary(N,BN.data,bth);
			bmWriteBlock(BN);				
			return ;
		}
		Block BNewRoot=bmReadBlock(fileName,N.sons[0]);
		node newRoot=binaryToNode(BNewRoot.data,bth);
		newRoot.isRoot=true;
		nodeToBinary(newRoot,BNewRoot.data,bth);
		bmWriteBlock(BNewRoot);

		bmReleaseBlock(fileName,BN.offset);

		bth.rootOffset=BNewRoot.offset;
		saveHeader(bth,fileName);
		return ;
	} else {

		nodeToBinary(N,BN.data,bth);
		bmWriteBlock(BN);				

		assert(N.sons.size()==N.data.size()+1);
		if (!N.isRoot && ((N.isLeaf && N.data.size()<n/2)  || (!N.isLeaf && N.sons.size()<(n+1)/2 )  )) {
			assert(searchpath.size()>=2);

			int parentoffset=*(searchpath.end()-2);

			Block BParent = bmReadBlock(fileName,parentoffset);
			node parent=binaryToNode(BParent.data,bth);
			Block BNP;
			element KP;
			bool Nisfront;
			bool findd=false;
			
			for (int i=0;i<parent.sons.size();i++) {
				if (parent.sons[i]==BN.offset) {
					findd=true;
					if (i==parent.sons.size()-1) {
						BNP=bmReadBlock(fileName,parent.sons[i-1]);
						KP=parent.data[i-1];
						Nisfront=false;
					} else {
						BNP=bmReadBlock(fileName,parent.sons[i+1]);
						KP=parent.data[i];
						Nisfront=true;
					}
				}
			}
			node NP=binaryToNode(BNP.data,bth);
			assert(findd);
			if (N.sons.size()+NP.sons.size()<=n) {
				if (Nisfront) {
					swap(N,NP);
					swap(BN,BNP);
				}
				if (!N.isLeaf) {
					NP.data.push_back(KP);
					for (int i=0;i<N.data.size();i++) 
						NP.data.push_back(N.data[i]);
					for (int i=0;i<N.sons.size();i++)
						NP.sons.push_back(N.sons[i]);
				} else {
					NP.sons.pop_back();
					for (int i=0;i<N.data.size();i++)
						NP.data.push_back(N.data[i]);
					for (int i=0;i<N.sons.size();i++)
						NP.sons.push_back(N.sons[i]);
				}
				nodeToBinary(N,BN.data,bth);
				nodeToBinary(NP,BNP.data,bth);
				bmWriteBlock(BN);
				bmWriteBlock(BNP);
				searchpath.pop_back();
				delete_entry(BParent,KP,BN.offset,fileName,bth);
				bmReleaseBlock(fileName,BN.offset);
			} else {
/*redistribute*/

				if (!Nisfront) {
// if N' -> N									
					if (!N.isLeaf) {
						int nppm=*(NP.sons.end()-1);
						element nkmm1=*(NP.data.end()-1);
						NP.sons.pop_back();
						NP.data.pop_back();
						N.sons.insert(N.sons.begin(),nppm);
						N.data.insert(N.data.begin(),KP);
						bool findd=false;
						for (int i=0;i<parent.data.size();i++) {
							if (parent.data[i]==KP) {
								findd=true;
								parent.data[i]=nkmm1;
								break;
							}
						}
						assert(findd);
						nodeToBinary(parent,BParent.data,bth);
						bmWriteBlock(BParent);
						nodeToBinary(N,BN.data,bth);
						bmWriteBlock(BN);
						nodeToBinary(NP,BNP.data,bth);
						bmWriteBlock(BNP);
					} else {
						int m=NP.data.size()-1;
						element npkm=*(NP.data.end()-1);

						N.data.insert(N.data.begin(),NP.data[m]);
						N.sons.insert(N.sons.begin(),NP.sons[m]);
						NP.data.erase(NP.data.begin()+m);
						NP.sons.erase(NP.sons.begin()+m);
						bool findd=false;
						for (int i=0;i<parent.data.size();i++) {
							if (parent.data[i]==KP) {
								findd=true;
								parent.data[i]=npkm;
							}
						}
						assert(findd);
						nodeToBinary(parent,BParent.data,bth);
						bmWriteBlock(BParent);
						nodeToBinary(N,BN.data,bth);
						bmWriteBlock(BN);
						nodeToBinary(NP,BNP.data,bth);
						bmWriteBlock(BNP);						
					}
				} else  {
//N ->N'
					if (!N.isLeaf) {
						int nppm=*(NP.sons.begin());
						element nkmm1=*(NP.data.begin());
						NP.sons.erase(NP.sons.begin());
						NP.data.erase(NP.data.begin());
						N.sons.push_back(nppm);
						N.data.push_back(KP);
						bool findd=false;
						for (int i=0;i<parent.data.size();i++) {
							if (parent.data[i]==KP) {
								findd=true;
								parent.data[i]=nkmm1;
								break;
							}
						}
						assert(findd);
						nodeToBinary(parent,BParent.data,bth);
						bmWriteBlock(BParent);
						nodeToBinary(N,BN.data,bth);
						bmWriteBlock(BN);
						nodeToBinary(NP,BNP.data,bth);
						bmWriteBlock(BNP);
					} else {
						int m=N.sons.size()-1;
						N.data.push_back(NP.data[0]);
						N.sons.insert(N.sons.begin()+m,NP.sons[0]);
						NP.data.erase(NP.data.begin());
						NP.sons.erase(NP.sons.begin());
						bool findd=false;
						for (int i=0;i<parent.data.size();i++) {
							if (parent.data[i]==KP) {
								findd=true;
								parent.data[i]=NP.data[0];
							}
						}
						assert(findd);
						nodeToBinary(parent,BParent.data,bth);
						bmWriteBlock(BParent);
						nodeToBinary(N,BN.data,bth);
						bmWriteBlock(BN);
						nodeToBinary(NP,BNP.data,bth);
						bmWriteBlock(BNP);						
					}
				}
			}
		} else {

		}
	}




	
}
bool btDelete(const string &fileName, const element &key) {

	BTreeHeader bth;
	Block block=bmReadBlock(fileName,0);
	memcpy(&bth,block.data,sizeof(bth));
	assert(bth.elementType==key.type);

	int offset=__find(fileName,bth.rootOffset,key,bth);
	block=bmReadBlock(fileName,offset);
	node now=binaryToNode(block.data,bth);	
	bool findd=false;
	element K;
	int P;
	for (int i=0;i<now.data.size();i++) {
		if (now.data[i]==key) {
			findd=true;
			K=key;
			P=now.sons[i];
		}
	}
	assert(findd);
	if (!findd)
		return false;
	delete_entry(block,K,P,fileName,bth);
	return true;
}

bool btExist(const string &fileName, const element &key) {

	BTreeHeader bth;
	Block block=bmReadBlock(fileName,0);
	memcpy(&bth,block.data,sizeof(bth));
	assert(bth.elementType==key.type);

	int offset=__find(fileName,bth.rootOffset,key,bth);
	block=bmReadBlock(fileName,offset);
	node now=binaryToNode(block.data,bth);	
	bool findd=false;
	element K;
	int P;
	for (int i=0;i<now.data.size();i++) {
		if (now.data[i]==key) {
			findd=true;
			K=key;
			P=now.sons[i];
		}
	}
	return findd;
}

set <int> btFindLess(const string &fileName, const element &key) {
	BTreeHeader bth;
	Block block=bmReadBlock(fileName,0);
	memcpy(&bth,block.data,sizeof(bth));
	if (bth.elementType!=key.type) {
		printf("%s %d %d\n",fileName.c_str(),bth.elementType,key.type);
	}
	assert(bth.elementType==key.type);
	
	int offset=bth.rootOffset;
	while (1) {
		node now=readNode(fileName,offset,bth);	
		if (now.isLeaf)
			break;
		offset=now.sons[0];
	}
	set <int> ret;
	while(offset!=-1) {
		node now=readNode(fileName,offset,bth);
		bool findd=false;		
		for (int i=0;i<now.data.size();i++) {
			if (now.data[i]<key) { 
				ret.insert(now.sons[i]);
			} else {
				findd=true;
				break;
			}
		}
		if (findd)
			break;
		offset=now.sons[now.sons.size()-1];
	}
	return ret;
}

set <int> btFindMore(const string &fileName, const element &key) {
	BTreeHeader bth;
	Block block=bmReadBlock(fileName,0);
	memcpy(&bth,block.data,sizeof(bth));
	assert(bth.elementType==key.type);
	
	int offset=__find(fileName,bth.rootOffset,key,bth);;

	set <int> ret;
	while(offset!=-1) {
		node now=readNode(fileName,offset,bth);
		for (int i=0;i<now.data.size();i++) {
			if (now.data[i]>key) { 
				ret.insert(now.sons[i]);
			} 
		}
		offset=now.sons[now.sons.size()-1];
	}
	return ret;
}
