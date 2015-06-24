#include <iostream>
#include <memory>
#include <random>
#include <vector>


#include "mcmalloc.h"

using namespace std;

int main(int argc, char *argv[])
{
	vector<void *>							ptrList;
	random_device 							rd;
	mt19937									g(rd());
	uniform_int_distribution<size_t>	length(1, 1024*1024);
	uniform_int_distribution<size_t>	element(0, 999);
	size_t									e, l;

	ptrList.assign(1000, nullptr);
	for(size_t i = 0; i < 100000; i++) {
		e = element(g);
		if(ptrList[e] == NULL) {
			l = length(g);
			ptrList[e] = malloc(l);
		} else {
			free(ptrList[e]);
		}
	}
	malloc_stats();
	for(size_t i = 0; i < 100000; i++) {
		if(ptrList[i] != nullptr) free(ptrList[i]);
	}
	return 0;
}