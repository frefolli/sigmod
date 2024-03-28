CXX=g++
CXXFLAGS=-Iinclude -std=c++11 -O3
TARGET=test
SRC=src/*.cc src/**/*.cc
INCLUDE=include/*.hh include/**/*.hh

all: $(TARGET)

$(TARGET): $(SRC) $(INCLUDE)
	$(CXX) $(CXXFLAGS) $(SRC) -o $@

dummy: $(TARGET)
	./$(TARGET) dummy-data.bin  dummy-queries.bin

contest: $(TARGET)
	./$(TARGET) contest-data-release-1m.bin contest-queries-release-1m.bin

clean:
	rm -f $(TARGET)
