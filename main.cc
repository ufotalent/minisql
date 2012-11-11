#include "interpreter.h"
#include <string>
#include <cstdio>
using namespace std;
char buf[1024];
int main() {
	do {
		printf(">>");
		gets(buf);
		ipAddLine(buf);
	}while (1);
}
