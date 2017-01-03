CXX=g++ -g

SRCS=main.cpp ssm.cpp graphutil.cpp graph.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

ssm: $(OBJS)
	$(CXX) -o ssm $(OBJS)
	
%.o: %.cpp ssm.hpp graphutil.hpp graph.hpp
	$(CXX) -c $<
