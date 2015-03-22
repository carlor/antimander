// shp2smg/islands.cpp
// Copyright (C) 2015 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#include "shp2smg.hpp"
#include "islands.hpp"
#include <algorithm>
#include <float.h>
#include <iostream>

void markChildren(Shpfile* shpfile, int* entIsland, int mark, int node) {
    if (entIsland[node] == -1) {
        entIsland[node] = mark;
        //std::cerr << "mark " << node << " #" << mark << std::endl;
        std::set<int> st = shpfile->edges[node];
        for(std::set<int>::iterator it = st.begin(); it != st.end(); it++) {
            markChildren(shpfile, entIsland, mark, *it);
        }
    }
}

int calc_newcomp(int* newcomp, int id) {
    if (newcomp[id] == id) return id;
    else return calc_newcomp(newcomp, newcomp[id]);
}

Kdnode::Kdnode(Point pt, bool sx, Kdnode*  l, Kdnode* r) {
    point = pt;
    splitx = sx;
    left = l;
    right = r;
}

Kdtree::Kdtree(const Point* points, size_t nPoints) {
    std::vector<Point> buffer(points, points+nPoints);
    bufbeg = buffer.begin();
    root = insert(true, buffer.begin(), buffer.end());
}

bool sortx(Point a, Point b) { return a.x < b.x; }
bool sorty(Point a, Point b) { return a.y < b.y; }

Kdnode* Kdtree::insert(bool splitx, PointIt beg, PointIt end) {
    int len = end-beg;
    if (len == 0) return NULL;
    if (len == 1) return new Kdnode(*beg, splitx, NULL, NULL);
    if (len > 1023) {
        std::cerr << beg-bufbeg << " to " << end-bufbeg << std::endl;
    }
    
    if (splitx) {
        std::sort(beg, end, sortx);
    } else {
        std::sort(beg, end, sorty);
    }
    
    PointIt mid = beg + len/2;
    return new Kdnode(*mid, splitx, insert(!splitx, beg, mid), insert(!splitx, mid+1, end));
}

/*
void Kdtree::insert(Point p) {
    root = insert(root, p, root ? root->splitx : false);
}

#define NODE_IS_LEFT node->splitx ? pt.x < node->point.x : pt.y < node->point.y
Kdnode* Kdtree::insert(Kdnode* node, Point pt, bool splitx) {
    if (node == NULL) {
        return new Kdnode(pt, splitx, NULL, NULL);
    } else if (node->point == pt && node->point.entity == pt.entity) {
        return node;
    } else {
        if (NODE_IS_LEFT) {
            node->left = insert(node->left, pt, !splitx);
        } else {
            node->right = insert(node->right, pt, !splitx);
        }
        return node;
    }
}

void Kdtree::remove(Point p) {
    root = remove(root, p);
}

Kdnode* Kdtree::remove(Kdnode* node, Point pt) {
    if (node == NULL) {
        return NULL;
    } else if (node->point == pt && node->point.entity == pt.entity) {
        Kdnode* left = node->left;
        Kdnode* right = node->right;
        bool axis = node->splitx;
        delete node;
        
        if (right == NULL) {
            return left;
        } else {
            Kdnode* parent = NULL;
            Kdnode* min = findMin(right, NULL, axis, &parent);
            if (parent) {
                parent->left = min->right;
            }
            min->splitx = axis;
            min->left = left;
            min->right = right;
            return min;
        }
    } else {
        if (NODE_IS_LEFT) {
            node->left = remove(node->left, pt);
        } else {
            node->right = remove(node->right, pt);
        }
        return node;
    }
}

Kdnode* Kdtree::findMin(Kdnode* node, Kdnode* parent, bool axis, Kdnode** fparent) {
    if (node->left == NULL) {
        *fparent = parent;
        return node;
    } else if (node->splitx == axis) {
        return findMin(node->left, node, axis, fparent);
    } else {        
        typedef Kdnode* kdp;
        kdp min1, fp1, min2, fp2;
        min1 = findMin(node->left, node, axis, &fp1);
        min2 = findMin(node->left, node, axis, &fp2);
        if (axis ? min1->point.x < min2->point.x : min1->point.y < min2->point.y) {
            return min1;
        } else {
            return min2;
        }
    }
}*/

int checks, blocks;
double Kdtree::findNearest(Point pt, Point* result, BlockFunc block, void* blockCtx, double maxd, Point* opp) {
    checks = blocks = 0;
    double r = findNearest(root, pt, result, block, blockCtx, maxd, opp);
    //std::cerr << checks << " checks" << std::endl;
    //std::cerr << blocks << " blocks" << std::endl;
    return r;
}

double Kdtree::findNearest(Kdnode* node, Point pt, Point* result, BlockFunc block, void* blockCtx, double min, Point* opp) {
    checks++;
    if (node == NULL) return min;
    
    double nd = node->point.dist2(pt);
    if (nd < min) {
        if (block(blockCtx, node->point.entity)) {
            blocks++;
        } else if (opp == NULL || nd < node->point.dist2(*opp)) {
            min = nd;
            *result = node->point; 
        }
    }
    
    double nap = node->splitx ? node->point.x : node->point.y;
    double pap = node->splitx ? pt.x : pt.y;
    if (pap - min < nap) {
        min = findNearest(node->left, pt, result, block, blockCtx, min, opp);
    }
    if (nap < pap + min) {
        min = findNearest(node->right, pt, result, block, blockCtx, min, opp);
    }
    
    return min;
}
