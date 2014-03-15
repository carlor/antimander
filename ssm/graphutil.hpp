// ssm/graphutil.hpp
// Copyright (C) 2014 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#ifndef SSMGRAPHUTILINCLUDE
#define SSMGRAPHUTILINCLUDE

#include "graph.hpp"
#include <limits.h>

typedef long long int DT;
const DT DT_UNMARKED = LLONG_MAX-1;

struct DD {
    size_t len;
    size_t root;
    DT* dist;
    size_t max(int* arr, int mark);
};

class DDDiff {
public:
    DDDiff(DD a, DD b);
    DT d(size_t v);
    bool operator() (size_t a, size_t b);
    bool markz;
private:
    DD a, b;
};

void findPseudoDiameter(SSMGraph* g, int* arr, int m0, DD* dds);
DD calcDD(SSMGraph* g, size_t vtx, int* arr, int m0);

int verifyCC(SSMGraph* g);
void markChildren(SSMGraph* g, bool* marked, size_t vtx);

#endif
