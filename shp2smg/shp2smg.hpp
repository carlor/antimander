// shp2smg/main.cpp
// Copyright (C) 2014 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#ifndef SHP2SMGINCLUDED
#define SHP2SMGINCLUDED

#include "frontiers.hpp"

#include <set>
#include <vector>

typedef unsigned long long Weight;

class Shpfile {
public:
    int load(const char* ifname);
    void calculateMST();
    void calculateNeighbors();
    void sbfYield(Frontier f, int freq);
    void connectIslands();
    int writeGraph();
    int writeCoasts();

    bool edgeBetween(int a, int b);
    void makeEdge(int a, int b);
    int readWeights(const char* ifname);
    int nEntities;
    Weight* weights;
    bool mst;

    Point* points;
    Frontiers frontiers;
    std::vector<Frontier> coasts;
    std::vector<std::set<int> > edges;
};

#endif
