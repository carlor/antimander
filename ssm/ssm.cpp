// ssm/ssm.cpp
// Copyright (C) 2014 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#include "ssm.hpp"
#include "graph.hpp"
#include "graphutil.hpp"

#include <assert.h>
#include <fstream>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

int ssmMain(int argc, char** argv) {
    std::queue<std::string> args;
    for(int i=0; i < argc; i++) args.push(argv[i]);

    int divs = 0;
    bool showHelp = false;
    std::string ddout;
    int r = 0;

    std::string pname = args.front();
    args.pop();
    while(!args.empty()) {
        std::string arg = args.front();
        args.pop();
        if (arg == "divide") {
            if (args.empty()) {
                std::cerr << "divide requires an argument." << std::endl;
                return 1;
            }

            std::stringstream divstr(args.front());
            divstr >> divs;
            args.pop();
            if (divs < 2) {
                std::cerr << "{divs} should be 2 or more." << std::endl;
                return 1;
            }
        } else if (arg == "--help" || arg == "-h" || arg == "help") {
            showHelp = true;
        } else if (arg == "--ddout") {
            if (args.empty()) {
                std::cerr << "ddout requires an argument." << std::endl;
                return 1;
            }

            ddout = args.front();
            args.pop();
        } else {
            std::cerr << "Unrecognized argument '" << arg << "'." << std::endl;
            showHelp = true;
            r = 1;
        }
    }

    if (divs == 0 && showHelp == false) {
        showHelp = true;
        r = 1;
    }

    if (showHelp) {
        std::cout << "Usage: " << pname << " [-h] divide {divs} [--ddout {dXd}]" << std::endl;
        std::cout << std::endl;
        std::cout << "    divide {divs}   divides into {divs} districts" << std::endl;
        std::cout << "    --ddout {dXd}    outputs DDs to file {dXd}" << std::endl;
        std::cout << "    --h, --help     shows this help" << std::endl;
        return r;
    }

    SSMGraph g;
    if (r = ssmRead(&g)) return r;
    int* sr = ssmDivide(&g, divs, ddout);
    if (!sr) return 1;
    return ssmWrite(&g, sr);
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
        std::cout << sr[i] << '\n';
    }
    std::cout << std::endl;
    return 0;
}

int* ssmDivide(SSMGraph* g, int divs, const std::string& ddout) {
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
    
    splitImpl(g, divs, size, arr, 0, mark, ddout);
    return arr;
}

void splitImpl(SSMGraph* g, int divs, Weight s0, int* arr, int m0, int& mn, const std::string& ddout) {
    if (divs > 1) {
        int d1 = divs / 2;
        int d2 = divs - d1;
        Weight s1 = (s0*d1)/divs;
        
        int m1 = mn++;
        
        s1 = dv3(g, arr, s1, m0, m1, ddout);
        //std::cerr << m0 << " => " << s0-s1 << ", ";
        //std::cerr << m1 << " => " << s1 << std::endl;
        splitImpl(g, d1, s1, arr, m1, mn, ddout);
        splitImpl(g, d2, s0-s1, arr, m0, mn, ddout);
    } else {
        std::cerr << "size of district " << m0 << " = " << s0 << std::endl;
    }
}

Weight dv3(SSMGraph* g, int* arr, Weight s1, int m0, int m1, const std::string& ddout) {
    size_t glen = g->length();

    std::cerr << "populating vlist..." << std::endl;
    std::vector<size_t> vlist;
    for(size_t v=0; v<glen; v++) {
        if (arr[v] == m0) {
            vlist.push_back(v);
        }
    }


    DD pd[2];
    findPseudoDiameter(g, arr, m0, pd);
    DDDiff ddd(pd[1], pd[0]);

    if (ddout.length()) {
        std::stringstream m1s;
        m1s << m1;
        
        std::string dds = ddout;
        dds.replace(dds.find('X'), 1, m1s.str());
        std::cerr << "recording " << dds << "..." << std::endl;
        std::ofstream df(dds.c_str());
        for (size_t i=0; i < glen; i++) {
            if (arr[i] == m0) {
                df << ddd.d(i) << "\n";
            } else {
                df << "0\n";
            }
        }
        df << std::endl;
        df.close();
    }

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
        DT df = ddd.d(v0);
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
