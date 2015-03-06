// shp2smg/main.cpp
// Copyright (C) 2014 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#include "shp2smg.hpp"

#include <assert.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <set>
#include <shapefil.h>
#include <stdlib.h>
#include <utility>

int Shpfile::load(const char* ifname) {
    std::cerr << "Loading graph..." << std::endl;

    SHPHandle hdl = SHPOpen(ifname, "rb");
    if (!hdl) {
        std::cerr << "Error opening file." << std::endl;
        return 1;
    }

    int shapetype;
    SHPGetInfo(hdl, &nEntities, &shapetype, NULL, NULL);


    if (shapetype != SHPT_POLYGON) {
        std::cerr << "shp2smg only supports polygons." << std::endl;
        SHPClose(hdl);
        return 1;
    }
    
    weights = new Weight[nEntities];

    int zero = 0;
    for(size_t i=0; i < nEntities; i++) {
        SHPObject* obj = SHPReadObject(hdl, i);
        if (obj) {
            weights[i] = 1;
            
            int* pstart;
            int parts = obj->nParts;
            if (parts == 0) {
                parts = 1;
                pstart = &zero;
            } else {
                pstart = obj->panPartStart;
            }
            
            for(int j=0; j < parts; j++) {
                int start = pstart[j];
                int end = (j+1 == parts) ? obj->nVertices : pstart[j+1];
                for(int k=start; k < end; k++) {
                    int l = k+1;
                    if (l == end) l = start;
                    frontiers.encounter(i,
                            Frontier(
                                Point(obj->padfX[k], obj->padfY[k], i),
                                Point(obj->padfX[l], obj->padfY[l], i)));
                }
            }
            
            SHPDestroyObject(obj);
        }
        edges.push_back(std::set<int>());

        if (i % 1024 == 0) {
            std::cerr << i << " / " << nEntities << std::endl;
        }
    }

    SHPClose(hdl);

    if (readWeights(ifname)) {
        std::cerr << "Failed to read weights, automatically set to 1." << std::endl;
    }

    return 0;
}

int Shpfile::readWeights(const char* ifname) {
    DBFHandle hdl = DBFOpen(ifname, "rb");
    if (!hdl) {
        std::cerr << "Couldn't open DBF file." << std::endl;
        return 1;
    }

    // TODO customizeable
    int wf = DBFGetFieldIndex(hdl, "POP10");
    if (wf == -1) {
        std::cerr << "Couldn't find weight field." << std::endl;
        DBFClose(hdl);
        return 1;
    }

    for (int i=0; i < nEntities; i++) {
        weights[i] = DBFReadIntegerAttribute(hdl, i, wf);
    }

    DBFClose(hdl);
    return 0;
}

struct Pairset {
    typedef std::set<std::pair<int, int> > Bkt;
    typedef Bkt::iterator iterator;
    Bkt backend;

    std::pair<int, int> mbs(int a, int b) {
        if (a > b) {
            int t = a;
            a = b;
            b = t;
        }
        return std::make_pair(a, b);
    }

    void add(int a, int b) {
        backend.insert(mbs(a, b));
    }

    bool find(int a, int b) {
        return backend.find(mbs(a, b)) != backend.end();
    }

    void remove(int a, int b) {
        backend.erase(mbs(a, b));
    }

    iterator begin() {
        return backend.begin();
    }

    iterator end() {
        return backend.end();
    }
};

void Shpfile::calculateNeighbors() {
    std::cerr << "calculating neighbors..." << std::endl;

    std::cerr << "sorting edges..." << std::endl;
    frontiers.sortByFrequency();
    std::cerr << "done sorting." << std::endl;
    
    FVec::iterator it, end;
    end = frontiers.inlands.end();
    for(it = frontiers.inlands.begin(); it != end; it++) {
        size_t nents = frontiers.freq(*it);
        if (nents > 2) {
            std::cerr << "threesome frontier at";
            ISet is = frontiers.entities(*it);
            for(ISet::iterator jt = is.begin(); jt != is.end(); jt++) {
                std::cerr << " " << *jt;
            }
            std::cerr << " " << it->a.x << " " << it->a.y
                      << " " << it->b.x << " " << it->b.y;
            std::cerr << std::endl;
        } else if (nents == 2) {
            ISet is = frontiers.entities(*it);
            std::vector<int> jt(is.begin(), is.end());
            //std::cerr << "n2 " << jt.size() << std::endl;
            int a = jt[0];
            int b = jt[1];
            if (!edgeBetween(a, b)) {
                makeEdge(a, b);
            }
        }
    }
}

int Shpfile::writeGraph() {
/*
    std::cerr << "writing coasts" << std::endl;
    SHPHandle hdl = SHPCreate(getenv("COASTS"), SHPT_ARC);
    std::ofstream of(getenv("COASTSLOG"));
    of.precision(10);
    for(int i=0; i < frontiers.coasts.size(); i++) {
        if (i % 1024 == 0) std::cerr << i << " / " << frontiers.coasts.size() << std::endl;
        double db[4];
        db[0] = frontiers.coasts[i].a.x;
        db[1] = frontiers.coasts[i].b.x;
        db[2] = frontiers.coasts[i].a.y;
        db[3] = frontiers.coasts[i].b.y;
        of << i << "\t" << db[0] << "\t" << db[1]
                << "\t" << db[2] << "\t" << db[3] << std::endl;
        SHPObject* obj = SHPCreateSimpleObject(SHPT_ARC, 2, db, db+2, NULL);
        SHPWriteObject(hdl, -1, obj);
        SHPDestroyObject(obj);
    }
    of.close();
    SHPClose(hdl);
*/
    
    std::cerr << "writing graph" << std::endl;
    std::cout << nEntities << std::endl;
    for(int i=0; i < nEntities; i++) {
        std::set<int> ed = edges[i];
        std::cout << weights[i] << " " << ed.size();
        for(std::set<int>::iterator j = ed.begin(); j != ed.end(); j++) {
            std::cout << " " << *j;
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    return 0;
}

bool Shpfile::edgeBetween(int a, int b) {
    return edges[a].find(b) != edges[a].end();
}

void Shpfile::makeEdge(int a, int b) {
    assert(!edgeBetween(a, b));
    edges[a].insert(b);
    edges[b].insert(a);
}


