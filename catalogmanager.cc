#include <cassert>
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <algorithm>
#include "table.h"
#include "catalogmanager.h"
#include "buffermanager.h"
#include "recordmanager.h"
using namespace std;

map <string,pair<string,int> > indexToTable;
class indexmanager {
public:
	indexmanager() {
		ifstream fin("index.catalog");
		if (!fin)
			return;
		string a,b;
		int c;
		while (fin>>a>>b>>c) {
			indexToTable[a]=make_pair(b,c);
		}
		fin.close();
	}
	~indexmanager() {
		ofstream fout("index.catalog");
		for (map <string,pair<string,int> >::iterator it=indexToTable.begin();it!=indexToTable.end();it++) {
			fout << it-> first << " "<< it ->second.first <<" "<< it->second.second<< endl;
		}
		fout.close();
	}
}indexmanagerinstance;
bool cmExistIndex(const string &indexname) {
	return indexToTable.find(indexname)!=indexToTable.end();
}
bool cmRegisterIndex(const string &tablename,const string &indexname, int itemIndex) {
	if (indexToTable.find(indexname)!=indexToTable.end())
		return false;
	indexToTable[indexname]=make_pair(tablename,itemIndex);
	return true;
}
pair<string,int> cmAskIndex(const string indexName) {
	assert(cmExistIndex(indexName));
	return indexToTable[indexName];
}
bool cmDeleteIndex(const string indexName) {
	if (indexToTable.find(indexName)==indexToTable.end())
		return false;
	indexToTable.erase(indexToTable.find(indexName));
	return true;
}
bool cmExistTable(const string &name) {
	ifstream fin((name).c_str());
	if (!fin)
		return false;
	string header;
	fin>>header;
	fin.close();
	return header==name;

}
table cmCreateTable(const string &name,const vector <item> &data) {
	assert(!cmExistTable(name));
	table newtable(name,data);
	newtable.write();
	return newtable;
}


table cmReadTable(const string &name) {
	assert(cmExistTable(name));
	table now;
	ifstream fin((name).c_str());
	fin >> now.name;
	fin >> now.entrySize;
	int itemsize;
	fin >> itemsize;
	for (int i=0;i<itemsize;i++) {
		item nitem;
		fin >> nitem.name;
		fin >> nitem.type;
		fin >> nitem.length;
		fin >> nitem.unique;
		int n;
		fin >> n;
		for (int j=0;j<n;j++) {
			string index;
			fin >> index;
			nitem.indices.insert(index);
		}
		now.items.push_back(nitem);
	}
	return now;
}

void cmDropTable(const string &name) {
	assert(cmExistTable(name));	
	string tablename=name;
	while (*(tablename.end()-1)!='.')
		tablename.erase(tablename.end()-1);
	tablename.erase(tablename.end()-1);

	table nowtable=cmReadTable(name);
	for (int i=0;i<nowtable.items.size();i++) {
		bool have=false;
		for (set<string>::iterator it=nowtable.items[i].indices.begin();it!=nowtable.items[i].indices.end();it++)  {
			cmDeleteIndex(*it);
			have=true;
		}
		if (have) bmClear(tablename+"."+nowtable.items[i].name+".index");
	}
	rmClear(tablename+".db");
	remove((name).c_str());
}
