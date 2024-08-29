# Target
TARGET = classifier
CXX = g++
CXXFLAGS = -lblas
SRCS = neuralNetwork.cc classifier.cc
OBJS = $(SRCS:.cpp=.o)
all:$(TARGET)
$(TARGET):$(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) 
%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

