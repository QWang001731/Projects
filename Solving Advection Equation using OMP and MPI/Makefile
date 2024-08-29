# Target
TARGET = ade_solver
CXX = g++
CXXFLAGS = -O3 -fopenmp
SRCS = Advection_equation_solver.cc
OBJS = $(SRCS:.cpp=.o)
all:$(TARGET)
$(TARGET):$(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) 
%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

