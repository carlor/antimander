// shp2smg/main.cpp
// Copyright (C) 2014 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#ifndef SHP2SMGINCLUDED
#define SHP2SMGINCLUDED

#include <set>
#include <vector>

typedef unsigned long long Weight;

struct Point {
    Point();
    Point(double x, double y, int entity);
    bool operator==(const Point& b) const;
    bool operator<(const Point& b) const;
    double x, y;
    int entity;
    double dist2(Point p);
};

bool point_lt(const Point& a, const Point& b);

struct Shpfile {
    int load(const char* ifname);
    void calculateNeighbors();
    void connectIslands();
    int writeGraph();

    bool edgeBetween(int a, int b);
    void makeEdge(int a, int b);
    int readWeights(const char* ifname);
    int nEntities;
    Weight* weights;

    std::vector<Point> points;
    std::vector<std::set<int> > edges;
};

#endif
