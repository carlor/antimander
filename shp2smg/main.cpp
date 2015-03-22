// shp2smg/main.cpp
// Copyright (C) 2014 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#include "shp2smg.hpp"

#include <iostream>

int main(int argc, char** argv) {
    bool mst = false;
    if (argc == 3 && (strcmp(argv[2], "--mst") || strcmp(argv[2], "--MST"))) {
        mst = true;
    } else if (argc != 2) {
        std::cerr << "Usage: " << argv[0]
            << " {arg}.shp > {arg}.smg" << std::endl;
        return 1;
    }

    Shpfile sp;
    sp.mst = mst;
    if (sp.load(argv[1])) return 1;
    if (mst) {
        sp.calculateMST();
    } else {
        sp.calculateNeighbors();
        sp.connectIslands();
    }
    return sp.writeGraph();
}
