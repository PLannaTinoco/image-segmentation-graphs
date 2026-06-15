CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall $(shell pkg-config --cflags opencv4)
LIBS = $(shell pkg-config --libs opencv4)

COMMON_SRC = src/common/image.cpp src/common/graph.cpp src/common/union_find.cpp

all: felzenszwalb cousty ift

felzenszwalb: $(COMMON_SRC) src/felzenszwalb/felzenszwalb.cpp src/felzenszwalb/main_felzenszwalb.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

cousty: $(COMMON_SRC) src/cousty/cousty.cpp src/cousty/main_cousty.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

ift: $(COMMON_SRC) src/ift/ift.cpp src/ift/main_ift.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

clean:
	rm -f felzenszwalb cousty ift *.o