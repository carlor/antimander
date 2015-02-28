CXX=g++ -g

SRCS=main.cpp shp2smg.cpp islands.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: build

build: $(OBJS)
	$(CXX) -lshp -o shp2smg $(OBJS)

%.o: %.cpp shp2smg.hpp islands.hpp
	$(CXX) -c $<
