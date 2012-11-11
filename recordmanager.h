#ifndef _RECORD_MANAGER_H_
#define _RECORD_MANAGER_H_

#include <string>
#include "table.h"
#include "element.h"
#include "fitter.h"
#include <vector>
using namespace std;
int rmInsertRecord(const string &fileName, const vector<element> &entry, const table &datatable);
void rmDeleteWithIndex(const string fileName,int offset, const Fitter &fitter, const table &datatable);
void rmDeleteWithoutIndex(const string fileName,const Fitter &fitter, const table &datatable);
vector <vector <element> > rmSelectWithIndex(const string fileName, int offset, const Fitter &fitter, const table &datatable);
vector <vector <element> > rmSelectWithoutIndex(const string fileName, const Fitter &fitter, const table &datatable);
void rmAddIndex(const string dbName,const string BTreeName, const table &datatable, int itemIndex);
set<int> rmGetAllOffsets(const string &fileName);
void rmClear(const string fileName) ;
#endif
