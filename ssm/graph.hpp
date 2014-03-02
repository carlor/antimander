// ssm/graph.hpp
// Copyright (C) 2014 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#ifndef SSMGRAPHINCLUDE
#define SSMGRAPHINCLUDE

#include <stdlib.h> // size_t

typedef unsigned long long Weight;

class SSMGraph {
    size_t len;
    Weight* weights;
    size_t* nbLens;
    size_t** neighbors;
    
public:
    void reset(size_t length);
    void resetVertex(size_t vtx, Weight weight, size_t nEdges);
    void setNeighbor(size_t vtx, size_t edge, size_t nb);
    
    size_t length();
    Weight weight(size_t vtx);
    size_t countNeighbors(size_t vtx);
    size_t* getNeighbors(size_t vtx);
};

#endif
