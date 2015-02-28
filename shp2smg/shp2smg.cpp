// shp2smg/main.cpp
// Copyright (C) 2014 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#include "islands.hpp"
#include "shp2smg.hpp"

#include <assert.h>
#include <float.h>
#include <iostream>
#include <math.h>
#include <set>
#include <shapefil.h>
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

    for(size_t i=0; i < nEntities; i++) {
        SHPObject* obj = SHPReadObject(hdl, i);
        if (obj) {
            weights[i] = 1;
            for(size_t j = 0; j < obj->nVertices; j++) {
                points.push_back(Point(obj->padfX[j], obj->padfY[j], i));
            }
            SHPDestroyObject(obj);
        }
        edges.push_back(std::set<int>());

        if (i % 1000 == 0) {
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

    std::cerr << "sorting points..." << std::endl;
    std::sort(points.begin(), points.end(), &point_lt);
    std::cerr << "done sorting." << std::endl;

    Pairset::Bkt encOnce;
    for (int i=0; i < points.size();) {
        int beg = i;
        Point pi = points[i];
        for(; i < points.size(); i++) {
            if (i % 1024 == 0) std::cerr << i << " / " << points.size() << std::endl;
            if (!(points[i] == pi)) break;
        }

        Pairset sofar;
        for(int j=beg; j < i; j++) {
            for(int k=j+1; k < i; k++) {
                sofar.add(points[j].entity, points[k].entity);
            }
        }

        for (Pairset::iterator j=sofar.begin(); j != sofar.end(); j++) {
            if (!edgeBetween(j->first, j->second)) {
                Pairset::iterator fc = encOnce.find(*j);
                if (fc == encOnce.end()) {
                    encOnce.insert(*j);
                } else {
                    encOnce.erase(*j);
                    makeEdge(j->first, j->second);
                }
            }
        }
    }
}

void Shpfile::connectIslands() {
    /*pseudocode:
    
    comps = find connected components
    sort comps by vertex count
    until len(comps) == 1:
        merge smallest comps to nearest comp
     */
    
    std::cerr << "calculating islands..." << std::endl;
    int* entIsland = new int[nEntities];
    for(size_t i=0; i < nEntities; i++) {
        entIsland[i] = -1;
    }
    
    int nComps = 0;
    for(size_t i=0; i < nEntities; i++) {
        if (entIsland[i] == -1) {
            std::cerr << "island " << nComps << ": " << i <<  std::endl;
            markChildren(this, entIsland, nComps++, i);
        }
    }
    
    std::cerr << nComps << " island(s) found..." << std::endl;
    if (nComps == 1) return;
    int* compSize = new int[nComps];
    for(size_t i=0; i < nComps; i++) compSize[i] = 0;
    for(size_t i=0; i < nEntities; i++) compSize[entIsland[i]]++;
    
#define ncp(ID) calc_newcomp(newcomp, (ID))
#define ncpe(EID) ncp(entIsland[(EID)])
    int* newcomp = new int[nComps];
    for(size_t i=0; i < nComps; i++) newcomp[i] = i;
    
    std::cerr << "building lists..." << std::endl;
    std::vector<int>* plists = new std::vector<int>[nComps];
    for(size_t i=0; i < points.size(); i++) {
        plists[entIsland[points[i].entity]].push_back(i);
    }
    
    std::cerr << "building tree..." << std::endl;
    Kdtree kdt(points);
    
    bool* block = new bool[nEntities];
    
    std::cerr << "joining islands...";
    while (nComps > 1) {
        //std::cerr << nComps << " islands left..." << std::endl;    
        // find smallest island
        int min = ncp(0);
        for(size_t i=1; i<nComps; i++) {
            int ni = ncp(i);
            if (compSize[ni] < compSize[min]) {
                min = ni;
            }
        }
        
        for(size_t i=0; i < nEntities; i++) {
            block[i] = (ncpe(i) == min);
        }
        
        Point bounds[4];
        bounds[0] = bounds[1] = bounds[2] = bounds[3] = points[plists[min][0]];
        for(size_t i=0; i < compSize[min]; i++) {
            Point p = points[plists[min][i]];
            if (p.x < bounds[0].x) bounds[0] = p;
            if (p.x > bounds[1].x) bounds[1] = p;
            if (p.y < bounds[2].y) bounds[2] = p;
            if (p.y > bounds[3].y) bounds[3] = p;
        }
        
        double maxd = DBL_MAX;
        int comp = 0;
        int pa, pb;
        for(size_t i=0; i < 4; i++) {
            Point p;
            double dist = kdt.findNearest(bounds[i], &p, block);
            if (dist < maxd) {
                pa = bounds[i].entity;
                pb = p.entity;
                comp = ncpe(pb);
                maxd = dist;
            }
        }
        
        std::cerr << "join " << comp << " to " << min << std::endl;
        newcomp[min] = comp;
        compSize[comp] += compSize[min];
        plists[comp].insert(plists[comp].end(), plists[min].begin(), plists[min].end());
        makeEdge(pa, pb);
        nComps--;
    }
}

int Shpfile::writeGraph() {
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

bool point_lt(const Point& a, const Point& b) {
    if (a.x < b.x) return true;
    if (a.x > b.x) return false;
    return a.y < b.y;
}

Point::Point() {
    this->x = nan("");
    this->y = nan("");
    this->entity = -1;
}

Point::Point(double x, double y, int entity) {
    // TODO make less hacky
    // correct longitudes
    if (x > 0) x -= 360;
    this->x = x;
    this->y = y;
    this->entity = entity;
}

// TODO tolerance
bool Point::operator==(const Point& b) const {
    return x == b.x && y == b.y;
}

bool Point::operator<(const Point& b) const {
    return point_lt(*this, b);
}

double Point::dist2(Point p) {
    double dx = x-p.x;
    double dy = y-p.y;
    return dx*dx + dy*dy;
}
