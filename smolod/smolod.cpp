// smolod/smolod.cpp
// Copyright (C) 2014 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#include "smolod.hpp"
#include "clipper.hpp"

#include <fstream>
#include <iostream>
#include <queue>
#include <stack>
#include <set>
#include <utility>
#include <stdlib.h>

#include <shapefil.h>

int main(int argc, char** argv) {
    std::queue<const char*> args;
    for(int i=0; i < argc; i++) args.push(argv[i]);

    bool showHelp = argc < 2;
    int r = 0;
    const char* smoFile;
    const char* dbfFile;
    const char* origFile;
    const char* dmapFile;
    const char* colname;

    const char* pname = args.front();
    args.pop();
    while(!args.empty()) {
        const char* arg = args.front();
        args.pop();
        // if arg endwithSmo set smoFile; if help duh; if .dbf load ids, if "dmap {shpfile}" duh, using clipper
        if (endswith(arg, ".smo")) {
            smoFile = arg;
        } else if (seq(arg, "-h") || seq(arg, "--help") || seq(arg, "help")) {
            showHelp = true;
        } else if (endswith(arg, ".dbf")) {
            dbfFile = arg;
            if (args.empty()) {
                std::cerr << "Column name not given." << std::endl;
                r = 1;
            } else {
                colname = args.front();
                args.pop();
            }
        } else if (endswith(arg, "dmap")) {
            if (args.size() < 2) {
                std::cerr << "Shapefile name not given." << std::endl;
                r = 1;
            } else {
                origFile = args.front();
                args.pop();
                dmapFile = args.front();
                args.pop();
            }
        }
    }

    if (!smoFile || (!dbfFile && !dmapFile && !showHelp)) {
        r = 1;
    }

    if (r || showHelp) {
        std::cout << "Usage: " << pname
            << " [-h] {ssmout.smo} [{blocks.dbf} {col}] [dmap {original.shp} {districts.shp}]"
            << std::endl;
        std::cout << std::endl;
        std::cout << "    {blocks.dbf} {col}                      add column to DBF file" << std::endl;
        std::cout << "    dmap {original.shp} {districts.shp}     create district map" << std::endl;
        std::cout << "    -h, --help                              shows this help" << std::endl;
        return r;
    }

    Smo smo(smoFile);

    if (dbfFile) {
        r |= loadIds(smo, dbfFile, colname);
    }

    if (dmapFile) {
        r |= createDistrictMap(smo, origFile, dmapFile);
    }

    return r;
}

Smo::Smo(const char* smoFile) {
    nDistricts = 0;
    std::ifstream inp(smoFile);
    while (true) {
        int dis;
        inp >> dis;
        if (!inp.good()) break;
        id2dis.push_back(dis);
        if (dis >= nDistricts) {
            nDistricts = dis+1;
        }
    }
}

int loadIds(const Smo& smo, const char* dbfFile, const char* colname) {
    DBFHandle hdl = DBFOpen(dbfFile, "rb+");
    if (!hdl) {
        std::cerr << "Error opening " << dbfFile << std::endl;
        return 1;
    }
    
    int nRecs = DBFGetRecordCount(hdl);
    if (nRecs > smo.countBlocks()) {
        std::cerr << "The size of the .smo (" << smo.countBlocks()
            << ") and the .dbf (" << nRecs << ") don't match."
            << std::endl;
        std::cerr << "@[n-1] = " << smo.districtOf(smo.countBlocks()-1) << std::endl;
        DBFClose(hdl);
        return 1;
    }
    
    int fid = DBFGetFieldIndex(hdl, colname);
    if (fid == -1) {
        fid = DBFAddField(hdl, colname, FTInteger, 10, 0);
        if (fid == -1) {
            std::cerr << "Error adding field " << colname << std::endl;
            DBFClose(hdl);
            return 1;
        }
    }
    
    for(int i=0; i < nRecs; i++) {
        int did = smo.districtOf(i);
        if (!DBFWriteIntegerAttribute(hdl, i, fid, did)) {
            std::cerr << "Error writing district " << did
                << " to record " << i << std::endl;
        }
    }
    
    DBFClose(hdl);
    return 0;
}

// TODO customizable
const double SCALE = 1.0e9;

int createDistrictMap(const Smo& smo, const char* origFile, const char* shpFile) {
    SHPHandle ohdl = SHPOpen(origFile, "rb");
    if (!ohdl) {
        return 1;
    }

    int nEntities, nShapetype;
    SHPGetInfo(ohdl, &nEntities, &nShapetype, NULL, NULL);
    if (nShapetype != SHPT_POLYGON) {
        std::cerr << "Must have polygons" << std::endl;
        SHPClose(ohdl);
        return 1;
    }
    if (nEntities > smo.countBlocks()) {
        std::cerr << "The size of the .smo (" << smo.countBlocks()
            << ") and the original.shp (" << nEntities << ") don't match."
            << std::endl;
        SHPClose(ohdl);
        return 1;
    }

    std::cerr << "Reading polygons..." << std::endl;
    PolyJoiner* pjs = new PolyJoiner[smo.countDistricts()];
    for (int i=0; i < nEntities; i++) {
        if (i % 1000 == 0) {
            std::cerr << i << " / " << nEntities << std::endl;
        }
        int did = smo.districtOf(i);
        SHPObject* sobj = SHPReadObject(ohdl, i);
        if (sobj) {
            addSobj(pjs[did], sobj);
        }
    }

    std::cout << "wkt_geom" << std::endl;
    SHPHandle dhdl = SHPCreate(shpFile, SHPT_POLYGON);
    for(int i=0; i < smo.countDistricts(); i++) {
        std::cerr << "calculating union of district " << i << "..." << std::endl;
        PolySet* ps = pjs[i].getPaths();
        int nv = ps->countTotalVertices();
        int nr = ps->countRings();
        int* rings = ps->ringIndices();
        DPoint* vtxs = ps->vertices();
        double* px = new double[nv];
        double* py = new double[nv];
        for(int j=0; j<nv; j++) {
            px[j] = vtxs[j].x;
            py[j] = vtxs[j].y;
        }
        delete[] vtxs;
        std::cerr << "nr = " << nr << std::endl;
        std::cout << "MULTIPOLYGON ((";
        for(int j=0; j<nr; j++) {
            std::cout << "(";
            int mx=(j+1==nr?nr:rings[j+1]);
            for (int k=rings[j]; k<mx; k++) {
                std::cout << vtxs[k].str();
                if (k+1<mx) std::cout << ", ";
            }
            std::cout << ")";
            if (j+1 < nr) {
                std::cout << ", ";
            }
        }
        std::cout << "))" << std::endl << std::endl;
        SHPObject* obj;
        if (nr == 1) {
            obj = SHPCreateObject(SHPT_POLYGON, i, 0, NULL, NULL, 
                                    nr==1?nv:rings[1], px, py, NULL, NULL);
        } else {
            int* rlist = new int[nr];
            for(int j=0; j<nr; j++) rlist[j] = SHPP_RING;
            obj = SHPCreateObject(SHPT_POLYGON, i, nr, rings, rlist,
                                    nv, px, py, NULL, NULL);
        }
        SHPComputeExtents(obj);
        SHPWriteObject(dhdl, -1, obj);
        
        /*std::queue<std::vector<DLine>*>* pts = pjs[i].getPaths();
        //if (pts.size() == 1) {
            std::vector<DLine>* pt = pts->front(); 
            int nv = pt->size();
            double* px = new double[nv];
            double* py = new double[nv];
            for(int j=0; j<nv; j++) {
                px[j] = (*pt)[j].left().x;// / SCALE;
                py[j] = (*pt)[j].left().y;// / SCALE;
            }
            delete pt;
            SHPObject* obj = SHPCreateObject(SHPT_POLYGON, i, 0, NULL, NULL, nv, px, py, NULL, NULL);
            SHPWriteObject(dhdl, -1, obj);
            /*} else {
            std::cerr << "multipolygons not implemented :(" << std::endl;
            std::cerr << "district " << i << " will be ignored" << std::endl;
            std::cerr << "pts.size() = " << pts.size() << std::endl;
        }*/
    }

    SHPClose(dhdl);
    SHPClose(ohdl);
    delete[] pjs;
    return 0;
}

int Smo::districtOf(int id) const {
    return id2dis[id];
}

int Smo::countDistricts() const {
    return nDistricts;
}

int Smo::countBlocks() const {
    return id2dis.size();
}

void addSobj(PolyJoiner& pj, SHPObject* sobj) {
    if (sobj->nParts == 0) {
        pj.add(sobj->nVertices, sobj->padfX, sobj->padfY);
    } else {
        /*if (sobj->panPartType[0] != SHPT_MULTIPATCH) {
            std::cerr << "Block " << sobj->nShapeId << " ill-formed." << std::endl;
        }*/
        int snp = sobj->nParts;
        for(int pi=0; pi < snp; pi++) {
            int beg = sobj->panPartStart[pi];
            int end = pi+1 == snp ? sobj->nVertices : sobj->panPartStart[pi+1];
            pj.add(end-beg, sobj->padfX+beg, sobj->padfY+beg);
        }
    }
}
/*
void PolyJoiner::add(PolyJoiner& pj) {
    ClipperLib::Clipper clp;
    clp.AddPaths(pj.pts, ClipperLib::ptSubject, true);
    clp.AddPaths(pts, ClipperLib::ptClip, true);
    ClipperLib::Paths npts;
    clp.Execute(ClipperLib::ctUnion, npts);
    pts = npts;
}*/

void PolyJoiner::add(int n, double* x, double* y) {
    //ClipperLib::Path p;
    //makePath(p, n, x, y);
    //pts << p;
    //exec(p, ClipperLib::ctUnion);
    
    for(int i=0; i<n; i++) {
        DPoint p(x[i], y[i]);
        int nid = i+1 == n ? 0 : i+1;
        DPoint p2(x[nid], y[nid]);
        DLine ln(p, p2);
        accum[ln] = accum[ln] + 1;
    }
}
/*
void PolyJoiner::remove(int n, double* x, double* y) {
    ClipperLib::Paths p;
    makePaths(p, n, x, y);
    exec(p, ClipperLib::ctDifference);
}*/

/*
int cmps=0;
bool sortx(const CLP& a, const CLP& b) {
    cmps++;
    if (cmps % 4096 == 0) {
        std::cerr << cmps << " compares" << std::endl;
    }
    return a.byx() < b.byx();
}

bool sorty(const CLP& a, const CLP& b) {
    cmps++;
    if (cmps % 4096 == 0) {
        std::cerr << cmps << " compares" << std::endl;
    }
    return b.byy() < b.byy();
}*/

template <typename T>
void append(std::vector<T>& dest, std::vector<T>& src) {
    dest.reserve(dest.size()+src.size());
    dest.insert(dest.end(), src.begin(), src.end());
}

struct PointMap {
    std::map<double, std::map<double, std::vector<int> > > bfr;
    void add(DLine dl, int id) {
        bfr[dl.left().x][dl.left().y].push_back(id);
        bfr[dl.right().x][dl.right().y].push_back(id);
    }
    
    std::set<int> neighbors(DLine dl, int id) {
        std::set<int> r;
        std::vector<int>* bp = &bfr[dl.left().x][dl.left().y];
        int nbl = bp->size();
        for(int i=0; i<nbl; i++) {
            int oid = (*bp)[i];
            if (oid != id) {
                r.insert(oid);
            }
        }
        bp = &bfr[dl.right().x][dl.right().y];
        nbl = bp->size();
        for(int i=0; i<nbl; i++) {
            int oid = (*bp)[i];
            if (oid != id) {
                r.insert(oid);
            }
        }
        return r;
    }
};



/*
struct pcmp {
    bool operator()(const DPoint& a, const DPoint& b) {
        return a.x*1e6+a.y < b.x*1e6+b.y;
    }
};*/

bool enc = false;
PolySet* PolyJoiner::getPaths() {
    /*
    ClipperLib::Clipper clp;
    clp.AddPaths(pts, ClipperLib::ptSubject, true);
    ClipperLib::Paths npts;
    clp.Execute(ClipperLib::ctUnion, npts);
    return npts;*/
    
    /*
    std::cerr << "initializing byx, byy..." << std::endl;
    int npts = pts.size();
    CLP* byx = new CLP[npts];
    CLP* byy = new CLP[npts];
    for(int i=0; i < npts; i++) {
        if (pts[i].size()) {
            byx[i].p = new ClipperLib::Path(pts[i]);
            byy[i].p = byx[i].p;
        } else {
            npts--;
        }
    }
    std::cerr << "sorting..." << std::endl;
    std::sort(byx, byx+npts, &sortx);
    std::sort(byy, byy+npts, &sorty);
    std::cerr << "done sorting..." << std::endl;
    int i=0;
    PJNode root(npts, byx, byy, true, i);
    ClipperLib::Paths r = root.join();
    delete[] byx, byy;
    return r;*/
    
    std::vector<DLine> borderLines;
    PointMap p2p;
    int i=0;
    for(typeof(accum.begin()) it = accum.begin(); it != accum.end(); it++) {
        if (it->second == 1) {
            DLine dl = it->first;
            borderLines.push_back(dl);
            p2p.add(dl, i++);
            if (!enc) {
                //std::cout << "LINESTRING (" << dl.left().x << " " << dl.left().y << ", " << dl.right().x << " " << dl.right().y << ")" << std::endl;
            }
        }
    }
    enc = true;
    
    PolySet* ps = new PolySet();
    int bls = borderLines.size();
    int* ccs = new int[bls];
    for(int i=0; i<bls; i++) ccs[i] = -1;
    int nccs = 0;
    for(int i=0; i < bls; i++) {
        if (ccs[i] == -1) {
            ps->startRing();
            int cc = nccs++;
            std::stack<int> q;
            q.push(i);
            while (!q.empty()) {
                int prev = q.top();
                q.pop();
                if (ccs[prev] < 0) {
                    ccs[prev] = cc;
                    DLine ln = borderLines[prev];
                    //lv->push_back(ln);
                    ps->addLine(ln);
                    std::set<int> nbs = p2p.neighbors(ln, i);
                    for(std::set<int>::iterator it=nbs.begin(); it != nbs.end(); it++) {
                        if (ccs[*it] == -1) {
                            q.push(*it);
                            ccs[*it] = -2;
                            goto end_inner_loop;
                        }
                    }
                    end_inner_loop:;
                }
                
                //prev = next;
                /*
                if (nbs.size() == 2) {
                    next = (nbs[0] == prev ? nbs[1] : nbs[0]);
                } else {
                    std::cerr << "malformed border" << std::endl;
                    return r;
                }*/
            }
            //r->push(lv);
            /*
            std::cerr << "front size: " << r.front()->size() << ", ";
            std::cerr << "push_back " << lv->size() << " in " << r.size();
            std::cerr << std::endl;*/
        }
    }
    
    return ps;
}

void PolySet::startRing() {
    rids.push_back(vtxs.size());
}

void PolySet::addLine(DLine ln) {
    int ridx = rids[rids.size()-1];
    if (vtxs.size() == ridx) {
        vtxs.push_back(ln.left());
        vtxs.push_back(ln.right());
    } else {
        int lidx = vtxs.size()-1;
        bool snd = lidx-ridx == 1;
        if (ln.left() == vtxs[lidx]) {
            vtxs.push_back(ln.right());
        } else if (ln.right() == vtxs[lidx]) {
            vtxs.push_back(ln.left());
        } else if (snd && ln.left() == vtxs[ridx]) {
            vtxs[ridx] = vtxs[lidx];
            vtxs[lidx] = ln.left();
            vtxs.push_back(ln.right());
        } else if (snd && ln.right() == vtxs[ridx]) {
            vtxs[ridx] = vtxs[lidx];
            vtxs[lidx] = ln.right();
            vtxs.push_back(ln.left());
        } else {
            std::cerr << "failure to concat " << ln.str() 
                      << " with " << DLine(vtxs[ridx], vtxs[lidx]).str()
                      << std::endl;
        }
    }
}

int PolySet::countTotalVertices() {
    return vtxs.size();
}

int PolySet::countRings() {
    return rids.size();
}

int* PolySet::ringIndices() {
    int* r = new int[rids.size()];
    for(int i=0; i<rids.size(); i++) r[i] = rids[i];
    return r;
}

DPoint* PolySet::vertices() {
    DPoint* r = new DPoint[vtxs.size()];
    for(int i=0; i<vtxs.size(); i++) r[i] = vtxs[i];
    return r;
}

/*
PJNode::PJNode(int np, CLP* byx, CLP* byy, bool xaxis, int& id) {
    this->xaxis = xaxis;
    this->left = NULL;
    this->right = NULL;
    if (np == 1) {
        path = new CLP;
        *path = *byx;
        idx = id++;
        if (idx % 1024 == 0) {
            std::cerr << "@ " << idx << std::endl;
        }
    } else {
        if (xaxis) {
            int md = np/2;
            CLP* newbyy = new CLP[np];
            int ldx=0, rdx=md;
            for(int i=0; i < np; i++) {
                if (byy[i].byx() < byx[md].byx()) {
                    newbyy[ldx++] = byy[i];
                } else {
                    newbyy[rdx++] = byy[i];
                }
            }
            this->left = new PJNode(md, byx, newbyy, false, id);
            this->right = new PJNode(np-md, byx+md, newbyy+md, false, id);
            delete[] newbyy;
        } else {
            int md = np/2;
            CLP* newbyx = new CLP[np];
            int ldx=0, rdx=md;
            for(int i=0; i<np; i++) {
                if (byx[i].byy() < byy[md].byy()) {
                    newbyx[ldx++] = byx[i];
                } else {
                    newbyx[rdx++] = byx[i];
                }
            }
            this->left = new PJNode(md, newbyx, byy, true, id);
            this->right = new PJNode(np-md, newbyx+md, byy+md, true, id);
            delete[] newbyx;
        }
    }
}

ClipperLib::Paths PJNode::join() {
    if (this->left) {
        if (idx % 1000 == 0) {
            std::cerr << "@ " << idx << std::endl;
        }
        return ::join(this->left->join(), this->right->join());
    } else {
        ClipperLib::Paths r;
        r.push_back(*this->path->p);
        return r;
    }
}

PJNode::~PJNode() {
    delete left, right;
    delete path;
}
*/

/*
void PolyJoiner::exec(ClipperLib::Paths& bpts, ClipperLib::ClipType ct) {
    ClipperLib::Clipper clp;
    clp.AddPaths(pts, ClipperLib::ptSubject, true);
    clp.AddPaths(bpts, ClipperLib::ptClip, true);
}*/

/*
void PolyJoiner::makePath(ClipperLib::Path& p, int n, double* x, double* y) {
    for(int i=0; i < n; i++) {
        const ClipperLib::IntPoint ip(x[i]*SCALE, y[i]*SCALE);
        p.push_back(ip);
    }
}

ClipperLib::Paths join(ClipperLib::Paths a, ClipperLib::Paths b) {
    ClipperLib::Clipper clp;
    ClipperLib::Paths r;
    clp.Execute(ClipperLib::ctUnion, r);
    return r;
}

void addSobjPath(ClipperLib::Clipper& clp, SHPObject* sobj, int beg, int end) {
    ClipperLib::Path p;
    for(int vi=beg; vi < end; vi++) {
        p << ClipperLib::IntPoint(sobj->padfX[vi]*SCALE, sobj->padfX[vi]*SCALE);
    }
    clp.AddPath(p, ClipperLib::ptSubject, true);
}
*/
bool endswith(const char* str, const char* suf) {
    int strl, sufl;
    strl = strlen(str);
    sufl = strlen(suf);
    return strcmp(str+strl-sufl, suf) == 0;
}

bool seq(const char* a, const char* b) {
    return strcmp(a, b) == 0;
}
