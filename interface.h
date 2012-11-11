#ifndef _INTERFACE_H_
#define _INTERFACE_H_
#include <string>
#include <vector>
#include "element.h"
#include "table.h"
#include "fitter.h"
using namespace std;

struct Response {
	bool succeed;
	string info;
	vector <vector <element> > result;
	inline Response() { succeed=true;}
	inline Response(string data) { succeed=false; info=data;}	
	inline Response(vector <vector <element> > result):result(result),succeed(true){}
};
Response CreateIndex(const string indexName,const string &tableName , const string &itemName);
Response DropIndex(const string indexName);
Response CreateTable(const string &tableName,vector <item> data,int pk);
Response DropTable(const string &tableName);
Response Select(const string &tableName, const Fitter &fitter);
Response Delete(const string &tableName, const Fitter &fitter);
Response Insert(const string tableName, const vector<element> entry);
#endif
