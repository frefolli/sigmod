CXX=g++
CXXFLAGS=-I. -std=c++11 -O3
TARGET=test
SRC=main.cc


all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(TARGET)
