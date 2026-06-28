CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wno-unused-result
INCLUDES = -I. -Isrcm -Ilibs
TARGET   = segmentar

$(TARGET): test_main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) test_main.cpp -o $(TARGET) -lm

clean:
	rm -f $(TARGET)
	rm -rf output/

.PHONY: clean
