#ifndef _TABLE_H_
#define _TABLE_H_
#include <set>
#include <string>
#include <vector>
using namespace std;
struct item {
	string name;
	int type;
	int length;
	bool unique;
	set<string> indices;
	bool operator ==(const item &rhs) const;
};
struct table {
	int entrySize;
	string name;
	vector <item> items;
	table (const string & name,const vector<item> &items);
	table ();	
	void write();
};

#endif
