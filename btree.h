#ifndef _BTREE_H_
#include "element.h"
void btCreate(const string &fileName,int type, int length=-1);
int btFind(const string &fileName,const element &key);
void btInsert(const string &filename,const element &key,int value);
bool btDelete(const string &fileName, const element &key);
void seetree(const string &filename);
bool btExist(const string &fileName, const element &key);
set <int> btFindLess(const string &fileName, const element &key);
set <int> btFindMore(const string &fileName, const element &key);
#define _BTREE_H_
#endif
