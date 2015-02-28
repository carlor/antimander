// shp2smg/main.cpp
// Copyright (C) 2014 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#include "shp2smg.hpp"

#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0]
            << " {arg}.shp > {arg}.smg" << std::endl;
        return 1;
    }

    Shpfile sp;
    if (sp.load(argv[1])) return 1;
    sp.calculateNeighbors();
    sp.connectIslands();
    return sp.writeGraph();
}
