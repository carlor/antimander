// ssm/graphutil.cpp
// Copyright (C) 2014 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#include "graphutil.hpp"
#include "graph.hpp"

#include <iostream>
#include <limits.h>
#include <set>
#include <utility>

struct PQComp {
    size_t vtx;
    int* dptr;
    
    PQComp(size_t v, int* p) {
        vtx = v;
        dptr = p;
    }
    
    bool operator< (const PQComp& b) const {
        return *dptr > *b.dptr;
    }
    
};

const int QSIZE = 16;

void findPseudoDiameter(SSMGraph* g, int* arr, int m0, DD* dds) {
    DD q[QSIZE];
    size_t nxtRoot = SIZE_MAX;
    for (size_t i = 0; i < QSIZE; i++) {
        q[i] = calcDD(g, nxtRoot, arr, m0);
        nxtRoot = q[i].max(arr, m0);
        for(size_t j=0; j < i; j++) {
            if (q[j].root == nxtRoot) {
                dds[0] = q[j];
                dds[1] = q[j+1];
                return;
            }
        }
    }
    dds[0] = q[QSIZE-2];
    dds[1] = q[QSIZE-1];
}

//typedef boost::heap::binomial_heap<PQComp> SPQ;

DD calcDD(SSMGraph* g, size_t base, int* arr, int m0) {
    size_t glen = g->length();
    int* dm = new int[glen];
    //SPQ::handle_type* hds = new SPQ::handle_type[glen];
    //SPQ* pq = new SPQ();
    std::set<std::pair<int, size_t> > q;
    
    //slog("initializing dm, pq...");
    for(size_t i=0; i<glen; i++) {
        if (arr[i] == m0) {
            if (base == SIZE_MAX || base == i) {
                base = i;
                dm[i] = 0;
            } else {
                dm[i] = INT_MAX-1; // because alt = dm[vtx] + 1
            }
            q.insert(std::make_pair(dm[i], i));
        }
    }
    
    std::cerr << "candidate " << base << std::endl;
    
    //slog("building dm...");
    while (!q.empty()) {
        size_t v = q.begin()->second;
        q.erase(q.begin());
    //std::cerr << "d " << base << " -> " << v << " = " << dm[v] << std::endl;
        size_t nnb = g->countNeighbors(v);
        size_t* nbs = g->getNeighbors(v);
        int alt = dm[v] + 1;
        for(size_t i=0; i < nnb; i++) {
            size_t nb = nbs[i];
            if (arr[nb] == m0 && alt < dm[nb]) {
                //std::cerr << "neighbor " << i << std::endl;
                q.erase(std::make_pair(dm[nb], nb));
                dm[nb] = alt;
                q.insert(std::make_pair(dm[nb], nb));
            }
        }
    }
    
    DD r;
    r.root = base;
    r.len = glen;
    r.dist = dm;
    return r;
}

size_t DD::max(int* arr, int m) {
    size_t k = 0;
    for(size_t i=0; i<len; i++) {
        if (arr[i] == m && dist[i] > dist[k]) {
            k = i;
        }
    }
    return k;
}

DDDiff::DDDiff(DD a, DD b) {
    this->a = a;
    this->b = b;
    markz = false;
}

int DDDiff::d(size_t v) {
    if (markz && v == 0) {
        std::cerr << "0! " << a.dist[0] << " " << b.dist[0] << std::endl;
    }
    return a.dist[v] - b.dist[v];
}

bool DDDiff::operator() (size_t a, size_t b) {
    return d(a) < d(b);
}

int verifyCC(SSMGraph* g) {
    size_t glen = g->length();

    bool* marked = new bool[glen];
    for (size_t i=0; i < glen; i++) {
        marked[i] = false;
    }

    markChildren(g, marked, 0);

    for (size_t i=0; i < glen; i++) {
        if (!marked[i]) {
            std::cerr << "Not a connected component; " << i;
            std::cerr <<" doesn't connect to 0." << std::endl;
            return 1;
        }
    }
    return 0;
}

void markChildren(SSMGraph* g, bool* marked, size_t vtx) {
    marked[vtx] = true;
    size_t cnb = g->countNeighbors(vtx);
    size_t* nbs = g->getNeighbors(vtx);
    for(size_t i=0; i < cnb; i++) {
        size_t nb = nbs[i];
        if (!marked[nb]) {
            markChildren(g, marked, nb);
        }
    }
}
