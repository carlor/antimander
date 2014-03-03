CXX=g++ -g

SRCS=main.cpp shp2smg.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: build

build: $(OBJS)
	$(CXX) -lshp -o shp2smg $(OBJS)

%.o: %.cpp shp2smg.hpp
	$(CXX) -c $<
