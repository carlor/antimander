CXX=g++ -g

SRCS=smolod.cpp clipper.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: build

build: $(OBJS)
	$(CXX) -lshp -o smolod $(OBJS)

%.o: %.cpp smolod.hpp clipper.hpp
	$(CXX) -c $<
