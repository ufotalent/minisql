#ifndef _CATALOG_MANAGER_H_
#define _CATALOG_MANAGER_H_

#include <string>
#include <algorithm>
#include "table.h"
using namespace std;
bool cmExistTable(const string &name);
table cmCreateTable(const string &name,const vector <item> &data);
table cmReadTable(const string &name);
void cmDropTable(const string &name);
bool cmExistIndex(const string &indexname);
bool cmRegisterIndex(const string &tablename,const string &indexname, int itemIndex);
pair<string,int> cmAskIndex(const string indexName);
bool cmDeleteIndex(const string indexName);
#endif
