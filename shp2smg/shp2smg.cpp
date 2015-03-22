// shp2smg/main.cpp
// Copyright (C) 2014 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#include "islands.hpp"
#include "shp2smg.hpp"

#include <assert.h>
#include <float.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <queue>
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
    if (mst) {
        points = new Point[nEntities];
    }

    int zero = 0;
    for(size_t i=0; i < nEntities; i++) {
        SHPObject* obj = SHPReadObject(hdl, i);
        weights[i] = 1;
        if (!obj) continue;
        if (mst) {
            // TODO better algorithm here
            points[i] = Point(
                (obj->dfXMin+obj->dfXMax)/2,
                (obj->dfYMin+obj->dfYMax)/2, i);
        } else {
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

struct UnionFind {
    int* cp;
    int* size;

    UnionFind(int N) {
        cp = new int[N];
        size = new int[N];
        for(int i=0; i<N; i++) {
            cp[i] = i;
            size[i] = 1;
        }
    }

    int comp(int i) {
        if (cp[i] == i) return i;
        cp[i] = cp[cp[i]];
        return comp(cp[i]);
    }

    void join(int a, int b) {
        a = comp(a);
        b = comp(b);
        if (size[a] > size[b]) {
            int t = a;
            a = b;
            b = t;
        }
        cp[a] = b;
        size[b] += size[a];
    }
};

bool bvk_block(void* ctx, int id) {
    void** ptrs = (void**)ctx;
    int a = *((int*)ptrs[0]);
    UnionFind* uf = (UnionFind*)ptrs[1];
    return uf->comp(a) == uf->comp(id);
}

struct CompNode {
    double min;
    int us, them;

    CompNode() { min = DBL_MAX; }
};

void Shpfile::calculateMST() {
    std::cerr << "Calculating MST..." << std::endl;

    UnionFind uf(nEntities);
    Kdtree tree(points, nEntities);

    int ufsize = nEntities;
    while(true) {
        std::map<int, CompNode> comps;
        for(int i=0; i<nEntities; i++) {
            if (i % 1024 == 0) {
                std::cerr << i << "\t/ " << nEntities
                          << "\t(" << ufsize << ")" << std::endl;
            }
            int comp = uf.comp(i);

            Point p;
            void* ctx[2];
            ctx[0] = &comp;
            ctx[1] = &uf;
            double dst = tree.findNearest(points[i], &p,
                                            &bvk_block, ctx,
                                            comps[comp].min);

            if (dst < comps[comp].min) {
                comps[comp].min = dst;
                comps[comp].us = i;
                comps[comp].them = uf.comp(p.entity);
            }
        }

        ufsize = comps.size();
        if (ufsize <= 1) break;

        for(std::map<int, CompNode>::iterator it = comps.begin();
                it != comps.end(); it++) {
            int comp = it->first;
            CompNode cn = it->second;
            if (!edgeBetween(cn.us, cn.them)) {
                makeEdge(cn.us, cn.them);
            }
            uf.join(comp, cn.them);
        }
    }
}

/*
// attempt 1 (roughly Prim's or Kruskal's algorithm)
struct PQNode {
    double dist;
    int vert, kin;

    bool operator<(const PQNode& that) const {
        return this->dist > that.dist;
    }

    bool operator==(const PQNode& that) const {
        return this->dist == that.dist;
    }
};

bool blockMarks(void* ctx, int id) {
    bool* marks = (bool*)ctx;
    return marks[id];
}

void Shpfile::calculateMST() {
    std::cerr << "calculating MST..." << std::endl;
    Kdtree tree(points, nEntities);
    bool* marked = new bool[nEntities];
    std::vector<int>* pointersTo = new std::vector<int>[nEntities];
    for(int i=0; i<nEntities; i++) marked[i] = false;
    std::priority_queue<PQNode> queue;
    Point p;

#define nst(ID, PT) tree.findNearest(points[(ID)], &p, &blockMarks, marked, DBL_MAX, (PT))
    marked[0] = true;
    PQNode nd, knd, vnd;
    nd.vert = 0;
    nd.dist = nst(0, NULL);
    nd.kin = p.entity;
    pointersTo[nd.kin].push_back(0);
    queue.push(nd);

    int mstSize = 1;
    int fails = 0;
    int ptts = 0;

    while (mstSize < nEntities) {
        if (mstSize % 1024 == 0) {
            //std::cerr << "--------" << std::endl;
            std::cerr << mstSize << " / " << nEntities << std::endl;
            //std::cerr << "--------" << std::endl;
        }
        nd = queue.top();
        queue.pop();

        if (marked[nd.kin]) {
            //std::cerr << "fail " << nd.vert << " -> " << nd.kin << std::endl;
            fails++;
            //nd.dist = nst(nd.vert);
            //if (nd.dist < DBL_MAX) {
            //    nd.kin = p.entity;
            //    queue.push(nd);
            //}
        } else {
            //std::cerr << nd.vert << " -> " << nd.kin << std::endl;
            makeEdge(nd.vert, nd.kin);
            marked[nd.kin] = true;
            mstSize++;

            knd.vert = nd.kin;
            knd.dist = nst(knd.vert, &points[nd.vert]);
            knd.kin = p.entity;
            pointersTo[knd.kin].push_back(knd.vert);
            queue.push(knd);

            std::vector<int> ptt = pointersTo[nd.kin];
            ptts += ptt.size();
            for(int i=0; i<ptt.size(); i++) {
                vnd.vert = ptt[i];
                vnd.dist = nst(ptt[i], &(points[knd.vert]));
                vnd.kin = p.entity;
                pointersTo[vnd.kin].push_back(vnd.vert);
                queue.push(vnd);
            }
        }
    }
    std::cerr << "fails:\t" << fails << std::endl;
    std::cerr << "ptts:\t" << ptts << std::endl;
    std::cerr << "mstsz:\t" << mstSize << std::endl;
}
*/

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

void sbf_yield(void* ctx, Frontier f, int freq);

void Shpfile::calculateNeighbors() {
    std::cerr << "calculating neighbors..." << std::endl;

    std::cerr << "sorting edges..." << std::endl;
    frontiers.sortByFrequency(sbf_yield, this);
    std::cerr << "done sorting." << std::endl;
    
    writeCoasts();
}

void sbf_yield(void* ctx, Frontier f, int freq) {   
    ((Shpfile*)ctx)->sbfYield(f, freq);
}

void Shpfile::sbfYield(Frontier f, int freq) {
    ISet is = frontiers.entities(f);
    if (freq > 2) {
        std::cerr << "threesome frontier at";
        for(ISet::iterator jt = is.begin(); jt != is.end(); jt++) {
            std::cerr << " " << *jt;
        }
        std::cerr << " " << f.a.x << " " << f.a.y
                  << " " << f.b.x << " " << f.b.y;
        std::cerr << std::endl;
    } else if (freq == 2) {
        std::vector<int> jt(is.begin(), is.end());
        //std::cerr << "n2 " << jt.size() << std::endl;
        int a = jt[0];
        int b = jt[1];
        if (!edgeBetween(a, b)) {
            makeEdge(a, b);
        }
    } else {
        // nents == 1
        coasts.push_back(f);
    }
}

bool ffn(void* ctx, int id) {
    int** nctx = (int**) ctx;
    return nctx[1][nctx[0][id]] == *(nctx[2]);
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
            //std::cerr << "island " << nComps << ": " << i <<  std::endl;
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

    std::cerr << "classifying coasts..." << std::endl;
    std::set<Point>* plists = new std::set<Point>[nComps];
    for(size_t i=0; i < coasts.size(); i++) {
        Frontier f = coasts[i];
        int fe = *frontiers.entities(f).begin();
        plists[entIsland[fe]].insert(f.a);
        plists[entIsland[fe]].insert(f.b);
    }
    
    std::cerr << "building tree..." << std::endl;
    size_t nPoints = 0;
    for(size_t i=0; i < nComps; i++) nPoints += plists[i].size();
    Point* points = new Point[nPoints];
    size_t pi=0;
    for(size_t i=0; i < nComps; i++) {
        for(std::set<Point>::iterator it = plists[i].begin(); it != plists[i].end(); it++) {
            points[pi++] = *it;
        }
    }
    Kdtree kdt(points, nPoints);
    
    std::cerr << "joining islands..." << std::endl;
    /*for(int i=0; i < nComps; i++) {
        std::cerr << plists[i].size() << " points for comp " << i << std::endl;
    }
    for(int i=0; i < nComps; i++) {
        std::cerr << "sz " << i << "\t" << compSize[i] << std::endl;
    }*/
    
    int min;
    int* ctx[3];
    ctx[0] = entIsland;
    ctx[1] = newcomp;
    ctx[2] = &min;
    
    int cleft = nComps;
    while (cleft > 1) {
        std::cerr << cleft << " islands left..." << std::endl;    
        // find smallest island
        min = ncp(0);
        for(size_t i=0; i<nComps; i++) {
            int ni = ncp(i);
            newcomp[i] = ni;
            if (compSize[ni] < compSize[min]) {
                min = ni;
            }
        }
        /*std::cerr << "i\tncp\tcs" << std::endl;
        for(int i=0; i<nComps; i++) {
            std::cerr << i << "\t" << newcomp[i] << "\t" << compSize[i] << "\t";
            if (newcomp[i] == i) std::cerr << "*";
            if (i == min) std::cerr << "<<";
            std::cerr << std::endl;
        }*/ 
        
        double maxd = DBL_MAX;
        int comp = 0;
        int pa, pb;
        std::set<Point> island = plists[min];
        std::set<Point>::iterator it;
        for(it = island.begin(); it != island.end(); it++) {
            Point p;
            double dist = kdt.findNearest(*it, &p, &ffn, ctx, maxd);
            if (dist < maxd) {
                pa = it->entity;
                pb = p.entity;
                comp = ncpe(pb);
                maxd = dist;
            }
        }
        
        std::cerr << "join " << min << " to " << comp
                  << " at " << pa << " to " << pb << std::endl;
        newcomp[min] = comp;
        compSize[comp] += compSize[min];
        plists[comp].insert(plists[min].begin(), plists[min].end());
        makeEdge(pa, pb);
        cleft--;
    }
}

int Shpfile::writeCoasts() {
    std::cerr << "writing coasts" << std::endl;
    const char* cfile = getenv("COASTS");
    if (cfile == NULL) cfile = "coasts_default.shp";
    const char* clogfile = getenv("COASTSLOG");
    if (clogfile == NULL) clogfile = "coasts_default_log.txt";
    SHPHandle hdl = SHPCreate(cfile, SHPT_ARC);
    if (!hdl) return 1;
    std::ofstream of(clogfile);
    of.precision(10);
    for(int i=0; i < coasts.size(); i++) {
        if (i % 1024 == 0) std::cerr << i << " / " << coasts.size() << std::endl;
        double db[4];
        db[0] = coasts[i].a.x;
        db[1] = coasts[i].b.x;
        db[2] = coasts[i].a.y;
        db[3] = coasts[i].b.y;
        of << i << "\t" << db[0] << "\t" << db[1]
                << "\t" << db[2] << "\t" << db[3] << std::endl;
        SHPObject* obj = SHPCreateSimpleObject(SHPT_ARC, 2, db, db+2, NULL);
        SHPWriteObject(hdl, -1, obj);
        SHPDestroyObject(obj);
    }
    of.close();
    SHPClose(hdl);
    return 0;
}

int Shpfile::writeGraph() {
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


