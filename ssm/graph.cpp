// ssm/graph.cpp
// Copyright (C) 2014 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#include "graph.hpp"

void SSMGraph::reset(size_t nVerts) {
    len = nVerts;
    weights = new Weight[len];
    nbLens = new size_t[len];
    neighbors = new size_t*[len];
}

void SSMGraph::resetVertex(size_t vtx, Weight weight, size_t nEdges) {
    weights[vtx] = weight;
    nbLens[vtx] = nEdges;
    neighbors[vtx] = new size_t[nEdges];
}

void SSMGraph::setNeighbor(size_t vtx, size_t edge, size_t nb) {
    neighbors[vtx][edge] = nb;
}

size_t SSMGraph::length() {
    return len;
}

Weight SSMGraph::weight(size_t vtx) {
    return weights[vtx];
}

size_t SSMGraph::countNeighbors(size_t vtx) {
    return nbLens[vtx];
}

size_t* SSMGraph::getNeighbors(size_t vtx) {
    return neighbors[vtx];
}

