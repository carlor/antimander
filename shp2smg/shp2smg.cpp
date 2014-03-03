// shp2smg/main.cpp
// Copyright (C) 2014 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#include "shp2smg.hpp"

#include <assert.h>
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
            if (i % 1000 == 0) std::cerr << i << " / " << points.size() << std::endl;
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

Point::Point(double x, double y, int entity) {
    this->x = x;
    this->y = y;
    this->entity = entity;
}

// TODO tolerance
bool Point::operator==(const Point& b) const {
    return x == b.x && y == b.y;
}
