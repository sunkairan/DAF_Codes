#include "Partition.h"
#include <iostream>
#include <string>

using namespace std;

const int numCls = 4;

void PrintContainer(Partition<string>& p);

int main() {
	Partition<string> p(numCls);

	//First data set
	p.insert("hello", 0);
	p.insert("jelly", 2);
	p.insert("bean", 2);
	p.insert("world", 0);
	p.insert("telegraph", 1);
	p.insert("test", 0);
	p.insert("yes", 0);
	p.insert("eclipse", 2);

	PrintContainer(p);
	cout << endl;
	cout << endl;
	
	//Change some class
	p.changeClass(6, 1);
	p.insert("millenium", 1);
	p.insert("rose", 0);
	p.insert("sunshine", 0);
	
	PrintContainer(p);
	cout << endl;
	cout << endl;
	
	//more change
	p.changeClass(3, 2);
	p.changeClass(5, 3);
	
	PrintContainer(p);
}

void PrintContainer(Partition<string>& p) {
	for (int i = 0; i < numCls; i++) {
		cout << "Content of class " << i << endl;
		for (ClassIterator<string> it = p.beginCls(i); it != p.endCls(i); ++it) {
			cout << *it << endl;
		}
		cout << "------------------" << endl;
	}
	cout << "Actual content of buffer" << endl;
	for (RangeIterator<string> it = p.begin(0, p.size() - 1); it != p.end(0, p.size() - 1); ++it) {
		cout << *it << endl;
	}
}

