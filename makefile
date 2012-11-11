final: main.o btree.o buffermanager.o element.o catalogmanager.o table.o recordmanager.o fitter.o interface.o interpreter.o
	g++ main.o btree.o buffermanager.o element.o catalogmanager.o table.o recordmanager.o fitter.o interface.o interpreter.o -o minisql
main.o: main.cc
	g++ main.cc -c 
btree.o: btree.cc btree.h
	g++ btree.cc -c 
buffermanager.o: buffermanager.cc buffermanager.h
	g++ buffermanager.cc -c 
element.o: element.cc element.h
	g++ element.cc -c 
catalogmanager.o: catalogmanager.cc catalogmanager.h
	g++ catalogmanager.cc -c 	
table.o: table.cc table.h
	g++ table.cc -c 		
recordmanager.o: recordmanager.cc recordmanager.h
	g++ recordmanager.cc -c
fitter.o: fitter.cc fitter.h
	g++ fitter.cc -c	
interface.o: interface.cc interface.h
	g++ interface.cc -c		
interpreter.o: interpreter.cc interpreter.h
	g++ interpreter.cc -c			

clean:
	rm -f *.o
	rm -f minisql
