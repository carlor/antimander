// ssm/ssm.hpp
// Copyright (C) 2014 Nathan M. Swan
// Available under the GNU GPL (see LICENSE)

#ifndef SSMSSMINCLUDE
#define SSMSSMINCLUDE

#include "graph.hpp"
#include <string>

int ssmMain(int argc, char** argv);
int ssmRead(SSMGraph* g);
int ssmWrite(SSMGraph* g, int* sr);

int* ssmDivide(SSMGraph* g, int divs, const std::string& ddout);
void splitImpl(SSMGraph* g, int divs, Weight s0, int* arr, int m0, int& mn, const std::string& ddout);
Weight dv3(SSMGraph* g, int* arr, Weight s1, int m0, int m1, const std::string& ddout);

#endif
