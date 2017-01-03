CXX=g++ -g

SRCS=main.cpp shp2smg.cpp islands.cpp frontiers.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

shp2smg: $(OBJS)
	$(CXX) -lshp -o shp2smg $(OBJS)

%.o: %.cpp shp2smg.hpp islands.hpp frontiers.hpp
	$(CXX) -c $<
