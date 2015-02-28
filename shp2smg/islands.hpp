// shp2smg/islands.hpp
// Copyright (C) 2015 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#ifndef ISLANDSINCLUDED
#define ISLANDSINCLUDED

#include "shp2smg.hpp"

void markChildren(Shpfile* shp, int* entIsland, int mark, int node);
int calc_newcomp(int* newcomp, int id);

struct Kdnode {
    Point point;
    bool splitx;
    Kdnode* left;
    Kdnode* right;
    
    Kdnode(Point pt, bool sx, Kdnode*  l, Kdnode* r);
};

typedef std::vector<Point>::iterator PointIt;

struct Kdtree {
public:
    Kdtree(const std::vector<Point>& points);
    //void insert(Point p);
    //void remove(Point p);
    double findNearest(Point pt, Point* result, bool* block);

private:
    Kdnode* root;
    PointIt bufbeg;
    
    Kdnode* insert(bool splitx, PointIt beg, PointIt end);
    //Kdnode* insert(Kdnode* nd, Point p, bool splitx);
    //Kdnode* remove(Kdnode* node, Point pt);
    //Kdnode* findMin(Kdnode* node, Kdnode* parent, bool axis, Kdnode** fparent);
    double findNearest(Kdnode* node, Point pt, Point* result, bool* block, double min);
};

#endif
