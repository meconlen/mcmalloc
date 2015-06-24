#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include <unistd.h>

#include "mcmalloc.h"

using namespace std;

int main(int argc, char *argv[])
{
	vector<void *>							ptrList;
	random_device 							rd;
	mt19937									g(rd());
	size_t									e, l;

	size_t	c, vectorLength = 1000, maxAllocation = 1024*1024, iterationCount = 1000000;

	while((c = getopt(argc, argv, "c:l:a:")) != -1) {
		switch(c) {
			case	'c':
				iterationCount = atol(optarg);
				break;
			case 	'l':
				vectorLength = atol(optarg);
				break;
			case 	'a': 
				maxAllocation = atol(optarg);
				break;
		}
	}

	uniform_int_distribution<size_t>	length(1, maxAllocation);
	uniform_int_distribution<size_t>	element(0, vectorLength - 1);

	ptrList.assign(vectorLength, nullptr);
	for(size_t i = 0; i < iterationCount; i++) {
		e = element(g);
		if(ptrList[e] == nullptr) {
			l = length(g);
			ptrList[e] = mc_kr_malloc(l);
			cout << "allocate: " << e << ": " << ptrList[e] << ", l = " << l << endl;
		} else {
			mc_kr_free(ptrList[e]);
			ptrList[e] = nullptr;
			cout << "free: " << e << ": " << ptrList[e] << endl;
		}
	}
	mc_kr_malloc_stats();
	for(size_t i = 0; i < vectorLength; i++) {
		if(ptrList[i] != nullptr) {
			mc_kr_free(ptrList[i]);
			cout << "free: " << i << ": " << ptrList[i] << endl;
		}
	}
	mc_kr_PrintFreelist();
	return 0;
}