#include <string>
#include <fstream>
#include <cassert>
#include "table.h"
using namespace std;

table::table (const string & name,const vector<item> &items) {
	entrySize=0;
	this->name=name;
	for (int i=0;i<items.size();i++) {
		switch (items[i].type) {
		case 0:entrySize+=sizeof(int);break;
		case 1:entrySize+=sizeof(float);break;			   
		case 2:entrySize+=items[i].length;break;
		default:assert(false);
			   
		}
	}
	this->items=items;
}
table::table() {

}
void table::write() {
	ofstream fout((name).c_str());
	fout << name << endl;
	fout << entrySize << endl;
	fout << items.size() << endl;
	for (int i=0;i<items.size();i++) {
		fout << items[i].name << endl;
		fout << items[i].type <<endl;
		fout << items[i].length << endl;
		fout << items[i].unique << endl;
		fout << items[i].indices.size();
		for (set <string>:: iterator it=items[i].indices.begin();it!=items[i].indices.end();it++) {
			fout  << " "<< *it;
		}
		fout <<  endl;
	}
	fout.close();
}
bool item::operator ==(const item &rhs) const {
	return type==rhs.type && length==rhs.length && unique==rhs.unique && indices==rhs.indices && name==rhs.name;
}
