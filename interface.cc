#include <vector>
#include <cstdio>
#include <cassert>
#include "interface.h"
#include "catalogmanager.h"
#include "recordmanager.h"
#include "buffermanager.h"
#include "btree.h"
using namespace std;

// File name rule:
// Table: $TableName.table
// Index: $TableName.$ItemName.index
// DB File: $TableName.db
Response CreateIndex(const string indexName,const string &tableName , const string &itemName) {
	if (!cmExistTable(tableName+".table")) {
		return Response("No such table");
	}
	table nowtable=cmReadTable(tableName+".table");
	int itemIndex=-1;
	for (int i=0;i<nowtable.items.size();i++) {
		if (nowtable.items[i].name==itemName) {
			itemIndex=i;
		}
	}
	if (itemIndex==-1) {
		return Response("No such item");
	}
	
	if (!nowtable.items[itemIndex].unique) {
		return Response("The item must be unique");
	}	
	if (!cmRegisterIndex(tableName,indexName,itemIndex)) {
		return Response("The index already exists");	
	}

	nowtable.items[itemIndex].indices.insert(indexName);
	nowtable.write();
	if (nowtable.items[itemIndex].indices.size()==1) {
		rmAddIndex(tableName+".db",tableName+"."+nowtable.items[itemIndex].name+".index",nowtable,itemIndex);
	}
	return Response();
}

Response DropIndex(const string indexName) {
	if (!cmExistIndex(indexName)) {
		return Response("No such index");
	}

	pair<string,int> psi=cmAskIndex(indexName);
	table nowtable=cmReadTable(psi.first+".table");
	assert(nowtable.items[psi.second].indices.count(indexName));
	nowtable.items[psi.second].indices.erase(indexName);
	nowtable.write();
	cmDeleteIndex(indexName);
	if (nowtable.items[psi.second].indices.size()==0)
		bmClear(psi.first+"."+nowtable.items[psi.second].name+".index");
	return Response();
}
Response CreateTable(const string &tableName,vector <item> data,int pk) {
	if (cmExistTable(tableName+".table")) {
		return Response("Table already exists");
	}

	table newtable=cmCreateTable(tableName+".table",data);
	newtable.items[pk].unique=true;
	newtable.write();
	return CreateIndex(tableName+".PrimaryKeyDefault",tableName,newtable.items[pk].name);
}

Response DropTable(const string &tableName) {
	if (!cmExistTable(tableName+".table")) {
		return Response("No such table");
	}
	cmDropTable(tableName+".table");
	return Response();
}

Response Select(const string &tableName, const Fitter &fitter) {
	if (!cmExistTable(tableName+".table")) {
		return Response("No such table");
	}
	const table nowtable=cmReadTable(tableName+".table");
	string dbname=tableName+".db";	
	for (int i=0;i<fitter.rules.size();i++) {
		element rhs=fitter.rules[i].rhs;
		int index=fitter.rules[i].index;
		if (nowtable.items[index].type!=rhs.type) {
			return Response("Type mismatch");
		}
	}	
	for (int i=0;i<fitter.rules.size();i++) {
		if (fitter.rules[i].type==2) {
			element rhs=fitter.rules[i].rhs;
			int index=fitter.rules[i].index;
			if (nowtable.items[index].indices.size()) { // has index on it
				string btreename=tableName+"."+nowtable.items[index].name+".index";
				int offset=btFind(btreename,rhs);
				return rmSelectWithIndex(dbname,offset,fitter,nowtable);
			}
		}
	}
	if (fitter.rules.size()==0) {
		return rmSelectWithoutIndex(dbname,fitter,nowtable);
	}

	set<int> offset=rmGetAllOffsets(tableName+".db");
	for (int i=0;i<fitter.rules.size();i++) {
		Rule rule=fitter.rules[i];

		int elementIndex=rule.index;
		if (!nowtable.items[elementIndex].indices.size()) 
			continue;
		element rhs=rule.rhs;
		string btreename=tableName+"."+nowtable.items[elementIndex].name+".index";
		set <int> newset;
		switch (rule.type) {
		case 0:newset=btFindLess(btreename,rhs);break;
		case 1:newset=btFindLess(btreename,rhs);newset.insert(btFind(btreename,rhs));break;
		case 2:assert(false);break;
		case 3:newset=btFindMore(btreename,rhs);newset.insert(btFind(btreename,rhs));break;
		case 4:newset=btFindMore(btreename,rhs);
		case 5:break;
		default:assert(false);
		}
		vector <int> tmp(offset.size());
		vector <int>::iterator end=set_intersection(offset.begin(),offset.end(),newset.begin(),newset.end(),tmp.begin());
		offset.clear();
		for (vector<int>::iterator it=tmp.begin();it!=end;it++) {
			offset.insert(*it);
		}
	}

	vector <vector <element> > res;
	for (set <int>::iterator it=offset.begin();it!=offset.end();it++) {
		vector <vector <element> > tmp=rmSelectWithIndex(tableName+".db",*it,fitter,nowtable);
		for (int i=0;i<tmp.size();i++) {
			res.push_back(tmp[i]);
		}
	}
	return Response(res);
}


Response Delete(const string &tableName, const Fitter &fitter) {
	if (!cmExistTable(tableName+".table")) {
		return Response("No such table");
	}
	const table nowtable=cmReadTable(tableName+".table");
	string dbname=tableName+".db";	
	for (int i=0;i<fitter.rules.size();i++) {
		element rhs=fitter.rules[i].rhs;
		int index=fitter.rules[i].index;
		if (nowtable.items[index].type!=rhs.type) {
			return Response("Type mismatch");
		}
	}
	for (int i=0;i<fitter.rules.size();i++) {
		if (fitter.rules[i].type==2) {
			element rhs=fitter.rules[i].rhs;
			int index=fitter.rules[i].index;
			if (nowtable.items[index].indices.size()) { // has index on it
				string btreename=tableName+"."+nowtable.items[index].name+".index";
				int offset=btFind(btreename,rhs);
				rmDeleteWithIndex(dbname,offset,fitter,nowtable);
				return Response();
			}
		}
	}
	if (fitter.rules.size()==0) {
		rmDeleteWithoutIndex(dbname,fitter,nowtable);
		return Response();
	}

	set<int> offset=rmGetAllOffsets(tableName+".db");
	for (int i=0;i<fitter.rules.size();i++) {
		Rule rule=fitter.rules[i];

		int elementIndex=rule.index;
		if (!nowtable.items[elementIndex].indices.size()) 
			continue;
		element rhs=rule.rhs;
		string btreename=tableName+"."+nowtable.items[elementIndex].name+".index";
		set <int> newset;
		switch (rule.type) {
		case 0:newset=btFindLess(btreename,rhs);break;
		case 1:newset=btFindLess(btreename,rhs);newset.insert(btFind(btreename,rhs));break;
		case 2:assert(false);break;
		case 3:newset=btFindMore(btreename,rhs);newset.insert(btFind(btreename,rhs));break;
		case 4:newset=btFindMore(btreename,rhs);
		case 5:break;
		default:assert(false);
		}
		vector <int> tmp(offset.size());
		vector <int>::iterator end=set_intersection(offset.begin(),offset.end(),newset.begin(),newset.end(),tmp.begin());
		offset.clear();
		for (vector<int>::iterator it=tmp.begin();it!=end;it++) {
			offset.insert(*it);
		}
	}
	for (set <int>::iterator it=offset.begin();it!=offset.end();it++) {
		rmDeleteWithIndex(tableName+".db",*it,fitter,nowtable);
	}
	return Response();
}

Response Insert(const string tableName, const vector<element> entry) {
	if (!cmExistTable(tableName+".table")) {
		return Response("No such table");
	}
	const table nowtable=cmReadTable(tableName+".table");
	string dbname=tableName+".db";	
	if (nowtable.items.size()!=entry.size())
		return Response("Type mismatch");
	for (int i=0;i<nowtable.items.size();i++) {
		if (nowtable.items[i].type!=entry[i].type) {
			return Response("Type mismatch");
		}
	}
	for (int i=0;i<nowtable.items.size();i++) {
		if (nowtable.items[i].unique) {
			Fitter fitter;
			fitter.addRule(Rule(i,2,entry[i]));
			Response tmp=Select(tableName,fitter);
			if (tmp.result.size()) 
				return Response("unique integrity violation");
		}
	}
	rmInsertRecord(tableName+".db",entry,nowtable);
	return Response();
}
