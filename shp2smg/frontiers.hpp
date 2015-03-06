// shp2smg/frontiers.hpp
// Copyright (C) 2015 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#ifndef FRONTIERS_HPP
#define	FRONTIERS_HPP

#include <map>
#include <set>
#include <vector>

struct Point {
    Point();
    Point(double x, double y, int entity);
    bool operator==(const Point& b) const;
    double x, y;
    int entity;
};

bool point_lt(const Point& a, const Point& b);

class Frontier {
public:
    Frontier(const Point& a, const Point& b);
//    int freq();
//    void encounter(int entity);
    
    bool operator==(const Frontier& that) const;
    bool operator<(const Frontier& that) const;
    
//    const std::set<int>& entities() const;
    Point a, b;
private:
//    int frequency;
//    std::set<int> ids;
    
};

typedef std::set<int> ISet;
typedef std::map<Frontier, ISet> FMap;
typedef std::vector<Frontier> FVec;

class Frontiers {
public:
    void encounter(int id, Frontier f);
    void sortByFrequency();
    std::vector<Frontier> coasts;
    std::vector<Frontier> inlands;
    int freq(Frontier f);
    ISet entities(Frontier f);
private:
    FMap backend;
};


#endif

