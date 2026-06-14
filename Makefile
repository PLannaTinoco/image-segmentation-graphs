CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

COMMON_SRC = src/common/image.cpp src/common/graph.cpp src/common/union_find.cpp

all: felzenszwalb cousty ift

felzenszwalb: $(COMMON_SRC) src/felzenszwalb/felzenszwalb.cpp src/felzenszwalb/main_felzenszwalb.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

cousty: $(COMMON_SRC) src/cousty/cousty.cpp src/cousty/main_cousty.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

ift: $(COMMON_SRC) src/ift/ift.cpp src/ift/main_ift.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f felzenszwalb cousty ift *.o
