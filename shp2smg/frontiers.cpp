
#include <iostream>
#include <set>
#include <vector>

#include "frontiers.hpp"

/**** points ****/
bool point_lt(const Point& a, const Point& b) {
    if (a.x < b.x) return true;
    if (a.x > b.x) return false;
    return a.y < b.y;
}

Point::Point() {
    
}

Point::Point(double x, double y, int entity) {
    this->x = x;
    this->y = y;
    this->entity = entity;
}


// TODO tolerance
bool Point::operator==(const Point& b) const {
   /* double xd = abs(x-b.x);
    double yd = abs(y-b.y);
    return xd+yd < 0.00000001;
    */return x == b.x && y == b.y;
}

/**** frontiers ****/
Frontier::Frontier(const Point& a, const Point& b) {
    if (point_lt(b, a)) {
        this->a = b;
        this->b = a;
    } else {
        this->a = a;
        this->b = b;
    }
//    frequency = 0;
}
/*
int Frontier::freq() {
    return frequency;
}

const std::set<int>& Frontier::entities() const {
    return ids;
}

void Frontier::encounter(int entity) {
    ids.insert(entity);
}
*/
bool Frontier::operator ==(const Frontier& that) const {
    return this->a == that.a && this->b == that.b;
}

bool Frontier::operator <(const Frontier& that) const {
    if (point_lt(this->a, that.a)) return true;
    if (point_lt(that.a, this->a)) return false;
    return point_lt(this->b, that.b);
}

void Frontiers::encounter(int id, Frontier f) {
    ISet vi;
    vi.insert(id);
    std::pair<FMap::iterator, bool> r
        = backend.insert(std::make_pair<Frontier, ISet>(f, vi));
    if (r.second == false) {
        r.first->second.insert(id);
    }
}
/*    std::set<Frontier>::iterator it = set.find(f);
    if (it == set.end()) {
        f.encounter(id);
        set.insert(f);
    } else {
        Frontier fi = *it;
        fi.encounter(id);
        set.erase(it);
        set.insert(it, fi);
    }
}*/

void Frontiers::sortByFrequency() {
    FMap::iterator it, end;
    end = backend.end();
    int size = backend.size();
    int i=0;
    for(it = backend.begin(); it != end; it++, i++) {
        if (i % 1024 == 0) std::cerr << i << " / " << size << std::endl;
        if (it->second.size() < 2) {
            coasts.push_back(it->first);
        } else {
            inlands.push_back(it->first);
        }
        
    }
}

int Frontiers::freq(Frontier f) {
    return backend[f].size();
}

ISet Frontiers::entities(Frontier f) {
    return backend[f];
}

