// smolod/smolod.hpp
// Copyright (C) 2014 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#ifndef SMOLODINCLUDE
#define SMOLODINCLUDE

#include <map>
#include <queue>
#include <vector>
#include <string>
#include <sstream>
#include <shapefil.h>
#include "clipper.hpp"

class Smo {
private:
    int nDistricts;
    std::vector<int> id2dis;

public:
    Smo(const char* smoFile);
    int districtOf(int id) const;
    int countDistricts() const;
    int countBlocks() const;
};

struct DPoint {
    double x, y;
    DPoint() {}
    DPoint(double bx, double by) { x = bx; y = by; }
    std::string str() { std::stringstream r; r<<x<<" "<<y; return r.str();}
    bool operator< (const DPoint& b) const {
        if (x < b.x) return true;
        if (x > b.x) return false;
        return y < b.y;
    }
    bool operator== (const DPoint& b) const { return x == b.x && y == b.y; }
};

class DLine {
private:
    DPoint a, b;
public:
    DLine(DPoint a, DPoint b) {
        if (a < b) {
            this->a = a;
            this->b = b;
        } else {
            this->b = a;
            this->a = b;
        }
    }
    std::string str() { return "(" + a.str() + ", " + b.str() + ")"; }
    DPoint left() { return a; }
    DPoint right() { return b; }
    bool operator< (const DLine& p) const { return a == p.a ? b < p.b : a < p.a; }
    bool operator== (const DLine& p) const { return a == p.a && b == p.b; }
};

class PolySet {
    std::vector<DPoint> vtxs;
    std::vector<int> rids;
public:
    void startRing();
    void addLine(DLine ln);
    int countTotalVertices();
    int countRings();
    int* ringIndices();
    DPoint* vertices();
};

class PolyJoiner {
private:
    std::map<DLine, int> accum;
    //void exec(ClipperLib::Paths& bpts, ClipperLib::ClipType ct);
    void makePath(ClipperLib::Path& p, int n, double* x, double* y);
public:
    //void add(PolyJoiner& pj);
    void add(int n, double* x, double* y);
    //void remove(int n, double* x, double* y);
    PolySet* getPaths();
};

int loadIds(const Smo& smo, const char* dbfFile, const char* colname);
int createDistrictMap(const Smo& smo, const char* origFile, const char* shpFile);

bool endswith(const char* str, const char* suf);
bool seq(const char* a, const char* b);

void addSobj(PolyJoiner& pj, SHPObject* sobj);
//void addSobjPath(ClipperLib::Clipper& clp, SHPObject* sobj, int beg, int end);


#endif
