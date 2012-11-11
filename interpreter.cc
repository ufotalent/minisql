
#include <cstdio>
#include <string>
#include <cstdlib>
#include <cstring>
#include <strstream>
#include "interface.h"
#include "interpreter.h"
#include "table.h"
#include "catalogmanager.h"

using namespace std;

void execfile(string filename) {
	FILE *fp=fopen(filename.c_str(),"r");
	if (!fp)
		printf("no such file\n");
	else {
		char buf[1024];
		while (fgets(buf,1024,fp)) {
			int len=strlen(buf);
			if (buf[len-1]=='\n') {
				buf[len-1]=0;
			}
			ipAddLine(buf);
		}
	}
}
element parseelement(string data) {
	char *c;
	if (data[0]=='\'') {
		string res;
		for (int i=1;i<data.length()-1;i++)
			res=res+data[i];
		return res;
	}
	if (data.find(".")!=-1) {
		return strtof(data.c_str(),&c);
	} else
		return int(strtod(data.c_str(),&c)+0.5);
}
void parse(string input) {
	if (input=="quit") {
		exit(0);
	}


	if (input=="")
		return;
	for (int i=0;i<input.length();i++) {
		if (input[i]=='(' || input[i]==')' || input[i]==',') {
			input.insert(i," ");
			input.insert(i+2," ");			
			i+=2;
		}
	}
	strstream str;	
	str << input;

	string operation;
	str >> operation;

	string useless;
	if (operation=="execfile") {
		string filename;
		str>>filename;
		execfile(filename);

	} else if (operation=="create"){ 
		string type;
		str>> type;
		if (type=="table") {
			string tablename;
			str>>tablename;
			str>>useless;
			vector <item> data;
			int pk=0x3f3f3f3f;
			while (1) {
				string itemname;
				str>>itemname;
				if (itemname=="primary") {
					str>>useless;
					str>>useless;
					string pkname;
					str>> pkname;
					for (int i=0;i<data.size();i++) {
						if (data[i].name==pkname) 
							pk=i;
					}
					break;
				}

				string datatype;
				int length=0;
				str>>datatype;
				if (datatype=="char") {
					str >>useless;
					str>>length;
					str>>useless;
				}
				string unique;
				str>>unique;
				item newitem;				
				if (unique=="unique") {
					newitem.unique=true;
					str>>useless;
				} else {
					newitem.unique=false;
				}
				newitem.name=itemname;
				if (datatype=="char") {
					newitem.type=2;
					newitem.length=length;
				} else if (datatype=="int"){
					newitem.type=0;
					newitem.length=0;
				} else if (datatype=="float"){
					newitem.type=1;
					newitem.length=0;
				} else {
					printf("unrecognized type %s\n",datatype.c_str()) ;
					return ;
				}		
				data.push_back(newitem);
			}
			if (pk==0x3f3f3f3f) {
				printf("primary key wrong");
			} else {
				Response res=CreateTable(tablename,data,pk);
				if (!res.succeed) {
					printf("%s\n",res.info.c_str());
				} else
					printf("OK\n");
			}

		} else if (type=="index") {
			string indexname;
			string tablename;
			string itemname;
			str>>indexname>>useless>>tablename>>useless>>itemname;

			Response res=CreateIndex(indexname,tablename,itemname);
			if (!res.succeed) {
				printf("%s\n",res.info.c_str());
			} else
				printf("OK\n");		
		} else printf("syntax error");

	} else if (operation=="drop"){
		string type;
		str >> type;
		if (type=="table") {
			string tablename;
			str >> tablename;
			Response res=DropTable(tablename);
			if (!res.succeed) {
				printf("%s\n",res.info.c_str());
			} else
				printf("OK\n");
		} else if (type=="index") {
			string indexname;
			str>>indexname;
			Response res=DropIndex(indexname);
			if (!res.succeed) {
				printf("%s\n",res.info.c_str());
			} else
				printf("OK\n");
		}
	
	} else if (operation=="insert") {
		string tablename;
		str >> useless >>tablename>>useless>>useless;
		vector <element> entry;
		while (1) {
			string data;
			str >> data;
			entry.push_back(parseelement(data));
			str >>useless;
			if (useless==")")
				break;
		}
		Response res=Insert(tablename,entry);
		if (!res.succeed) {
			printf("%s\n",res.info.c_str());
		} else
			printf("OK\n");

	} else if (operation=="select") {
		string tablename;
		str >> useless >> useless >> tablename;
		Fitter fitter;
		if (!cmExistTable(tablename+".table")) {
			printf("no such table\n");
			return;
		}
		table nowtable=cmReadTable(tablename+".table");
		while (str>>useless) {
			string itemname;
			string oper;
			string data;
			str >> itemname >> oper >> data;
			int index=-1;
			for (int i=0;i<nowtable.items.size();i++) {
				if (nowtable.items[i].name==itemname) {
					index=i;
					break;
				}
			}
			if (index<0)  {
				printf("no such item\n");
				return ;
			}
			int op;
			if (oper=="<") {
				op=0;
			} else if (oper=="<=") {
				op=1;
			} else if (oper=="=") {
				op=2;
			} else if (oper==">=") {
				op=3;
			} else if (oper==">") {
				op=4;
			} else if (oper=="<>") {
				op=5;
			} else {
				printf("syntax error\n");
				return ;
			}
			fitter.addRule(Rule(index,op,parseelement(data)));
		}
		Response res=Select(tablename,fitter);
		if (!res.succeed) 
			printf("%s\n",res.info.c_str());
		else {

			vector <int> space(nowtable.items.size());
			for (int i=0;i<nowtable.items.size();i++) {
				space[i]=nowtable.items[i].name.length()+1;
			}
			for (int i=0;i<res.result.size();i++) {
				for (int j=0;j<res.result[i].size();j++) {
					int len;
					switch (res.result[i][j].type)  {
						case 0:{str << res.result[i][j].datai;string s;str>>s;len=s.length(); break;}
						case 1:{str << res.result[i][j].dataf;string s;str>>s;len=s.length(); break;}
						case 2:{len=res.result[i][j].datas.length();break;}
					}
					if (len>space[j])
						space[j]=len+1;
				}
			}
			int sum=0;
			for (int i=0;i<nowtable.items.size();i++) 
				sum+=space[i]+1;
			sum++;
			for (int i=0;i<sum;i++) printf("-");
			printf("\n");
			printf("|");
			for (int i=0;i<nowtable.items.size();i++) {
				printf("%s",nowtable.items[i].name.c_str());
				for (int j=0;j<space[i]-nowtable.items[i].name.length();j++)
					printf(" ");
				printf("|");
			}
			printf("\n");
			for (int i=0;i<res.result.size();i++) {
				printf("|");
				for (int j=0;j<res.result[i].size();j++) {
					int len;
					strstream str;
					switch (res.result[i][j].type)  {
						case 0:{str << res.result[i][j].datai;string s;str>>s;len=s.length(); break;}
						case 1:{str << res.result[i][j].dataf;string s;str>>s;len=s.length(); break;}
						case 2:{len=res.result[i][j].datas.length();break;}
					}
					res.result[i][j].print();
					for (int s=0;s<space[j]-len;s++) 
						printf(" ");
					printf("|");
				}
				printf("\n");
			}
			for (int i=0;i<sum;i++) printf("-");
			printf("\n");
		}
	} else if (operation=="delete") {
		string tablename;
		str >> useless >> tablename;
		Fitter fitter;
		if (!cmExistTable(tablename+".table")) {
			printf("no such table\n");
			return;
		}
		table nowtable=cmReadTable(tablename+".table");
		while (str>>useless) {
			string itemname;
			string oper;
			string data;
			str >> itemname >> oper >> data;
			int index=-1;
			for (int i=0;i<nowtable.items.size();i++) {
				if (nowtable.items[i].name==itemname) {
					index=i;
					break;
				}
			}
			if (index<0)  {
				printf("no such item\n");
				return ;
			}
			int op;
			if (oper=="<") {
				op=0;
			} else if (oper=="<=") {
				op=1;
			} else if (oper=="=") {
				op=2;
			} else if (oper==">=") {
				op=3;
			} else if (oper==">") {
				op=4;
			} else if (oper=="<>") {
				op=5;
			} else {
				printf("syntax error\n");
				return ;
			}
			fitter.addRule(Rule(index,op,parseelement(data)));
		}
		Response res=Delete(tablename,fitter);
		if (!res.succeed) 
			printf("%s\n",res.info.c_str());
	} else {
		puts("syntax error");
	}


}
void ipAddLine(string now) {

	static string data;	
	for (int i=0;i<now.length();i++) {
		if (now[i]==';') {
			string dd=data;
			data="";
			parse(dd); 
		} else {
			data=data+now[i];
		}
	}
}
