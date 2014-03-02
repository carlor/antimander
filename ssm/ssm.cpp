// ssm/ssm.cpp
// Copyright (C) 2014 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#include "ssm.hpp"
#include "graph.hpp"
#include "graphutil.hpp"

#include <assert.h>
#include <iostream>
#include <vector>

int ssmMain(int argc, char** argv) {
	if (argc >= 3 && !strcmp(argv[1], "divide")) {
        int divs = atoi(argv[2]);
        if (divs < 2) {
            std::cerr << "{divs} should be 2 or more." << std::endl;
            return 1;
        }
        
        SSMGraph g;
        if (int r = ssmRead(&g)) return r;
        int* sr = ssmDivide(&g, divs);
        if (!sr) return 1;
        return ssmWrite(&g, sr);
	} else {
	    std::cerr << "Usage: " << argv[0] << " divide {divs}" << std::endl;
        return 1;
	}
}

int ssmRead(SSMGraph* g) {
    size_t nVerts;
    std::cin >> nVerts;
    g->reset(nVerts);
    
    for (size_t i=0; i<nVerts; i++) {
        Weight weight;
        std::cin >> weight;
        
        size_t nEdges;
        std::cin >> nEdges;
        g->resetVertex(i, weight, nEdges);
        
        for (size_t j=0; j < nEdges; j++) {
            size_t nb;
            std::cin >> nb;
            g->setNeighbor(i, j, nb);
        }
    }

    return verifyCC(g);
}

int ssmWrite(SSMGraph* g, int* sr) {
    size_t len = g->length();
    for (size_t i=0; i<len; i++) {
        std::cout << sr[i] << ' ';
    }
    std::cout << std::endl;
    return 0;
}

int* ssmDivide(SSMGraph* g, int divs) {
    size_t glen = g->length();
    
    std::cerr << "initializing arr..." << std::endl;
    int* arr = new int[glen];
    int mark = 1;
    for (size_t i=0; i<glen; i++) {
        arr[i] = 0;
    }
    
    std::cerr << "calculating size..." << std::endl;
    Weight size = 0;
    for (size_t i=0; i<glen; i++) {
        size += g->weight(i);
    }
    
    splitImpl(g, divs, size, arr, 0, mark);
    return arr;
}

void splitImpl(SSMGraph* g, int divs, Weight s0, int* arr, int m0, int& mn) {
    if (divs > 1) {
        int d1 = divs / 2;
        int d2 = divs - d1;
        Weight s1 = (s0*d1)/divs;
        
        int m1 = mn++;
        
        s1 = dv3(g, arr, s1, m0, m1);
        //std::cerr << m0 << " => " << s0-s1 << ", ";
        //std::cerr << m1 << " => " << s1 << std::endl;
        splitImpl(g, d1, s1, arr, m1, mn);
        splitImpl(g, d2, s0-s1, arr, m0, mn);
    } else {
        std::cerr << "size of district " << m0 << " = " << s0 << std::endl;
    }
}

Weight dv3(SSMGraph* g, int* arr, Weight s1, int m0, int m1) {
    size_t glen = g->length();
    
    DD pd[2];
    findPseudoDiameter(g, arr, m0, pd);
    
    // TODO better heuristic
    /*
    slog("finding pd1...");
    DD root = calcDD(g, SIZE_MAX, arr, m0);
    size_t pd1r = root.max();
    DD pd1 = calcDD(g, pd1r, arr, m0);
    
    slog("finding pd0...");
    size_t pd0r = pd1.max();
    DD pd0 = calcDD(g, pd0r, arr, m0);*/
    
    std::cerr << "populating vlist..." << std::endl;
    std::vector<int> vlist;
    for(size_t v=0; v<glen; v++) {
        if (arr[v] == m0) {
            vlist.push_back(v);
        }
    }
    
    //std::cerr << "~s0: " << i << ", s0: " << s0 << std::endl;
    
    DDDiff ddd(pd[1], pd[0]);
    
    std::cerr << "sorting vlist..." << std::endl;
    std::sort(vlist.begin(), vlist.end(), ddd);
    
    ddd.markz = true;
    size_t i = 0;
    Weight m = 0;
    while (i < vlist.size()) {
        std::cerr << "m / s1 = " << m << " / " << s1 << std::endl;
        
        size_t prp = i; // where counting began
        size_t v0 = vlist[i];
        std::cerr << v0 << std::endl;
        int df = ddd.d(v0);
        Weight mn = g->weight(v0);
        i++;
        
        while (i < vlist.size()) {
            size_t vi = vlist[i];
            if (ddd.d(vi) == df) {
                mn += g->weight(vi);
                i++;
            } else {
                break;
            }
        }
        
        Weight mp = m + mn;
        if (mp < s1 || (mp-s1 < s1-m)) {
            m += mn;
            for(size_t j=prp; j < i; j++) {
                arr[vlist[j]] = m1;
            }
        }
        
        if (mp >= s1) {
            return m;
        }
    }
    assert(0);
    return s1 * 3;
}
