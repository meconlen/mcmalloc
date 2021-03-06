#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include <unistd.h>

#include "mcmalloc.h"

using namespace std;

vector<size_t>	lambda = {64, 128, 1024, 1024*1024, 4*1024*1024};
vector<double>	lambdap = {0.80, 0.10, 0.05, 0.03, 0.02};

size_t getLength() {
	static 	random_device	rd;
	static 	mt19937			g(rd());

	static poisson_distribution<size_t> 		d[5] = { 
		poisson_distribution<size_t>(lambda[0]),
		poisson_distribution<size_t>(lambda[1]),
		poisson_distribution<size_t>(lambda[2]),
		poisson_distribution<size_t>(lambda[3]),
		poisson_distribution<size_t>(lambda[4])
	};
	static uniform_real_distribution<double>	which(0, 1.0);
	double 										w;
	double 										sum;
	int 										i;

	w = which(g);
	sum=0;
	for(i=0; i<5; i++) {
		sum+=lambdap[i];
		if(sum > w) break;
	}
	return d[i](g);
}

int main(int argc, char *argv[])
{
	vector<void *>							ptrList;
	random_device 							rd;
	mt19937									g(rd());
	size_t									e, l;

	size_t	vectorLength = 1000, lambda = 48, iterationCount = 1000000;
	size_t	maxLength = 0;
	int 		c;

	while((c = getopt(argc, argv, "c:l:a:")) != -1) {
		switch(c) {
			case	'c':
				iterationCount = atol(optarg);
				break;
			case 	'l':
				vectorLength = atol(optarg);
				break;
			case 	'a': 
				lambda = atol(optarg);
				break;
		}
	}

	// uniform_int_distribution<size_t>	length(1, maxAllocation);
	poisson_distribution<size_t> length(lambda);
	uniform_int_distribution<size_t>	element(0, vectorLength - 1);

	ptrList.assign(vectorLength, nullptr);
	for(size_t i = 0; i < iterationCount; i++) {
		e = element(g);
		if(ptrList[e] == nullptr) {
			// l = length(g);
			l = getLength();
			if(l > maxLength) {
				maxLength = l;
				cout << "max length = " << maxLength << endl;
			}
			ptrList[e] = malloc(l);
			// cout << "allocate(" << i << "): " << e << ": " << ptrList[e] << ", l = " << l << endl;
		} else {
			free(ptrList[e]);
			ptrList[e] = nullptr;
			// cout << "free(" << i << "): " << e << ": " << ptrList[e] << endl;
		}
	}
#if defined(HAVE_MALLOC_STATS)
	malloc_stats();
#endif
	for(size_t i = 0; i < vectorLength; i++) {
		if(ptrList[i] != nullptr) {
			free(ptrList[i]);
			// cout << "free(" << i << "): " << i << ": " << ptrList[i] << endl;
		}
	}
	return 0;
}