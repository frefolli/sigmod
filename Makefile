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

contest-1m: $(TARGET)
	./$(TARGET) contest-data-release-1m.bin contest-queries-release-1m.bin

contest-10m: $(TARGET)
	./$(TARGET) contest-data-release-10m.bin contest-queries-release-10m.bin

clean:
	rm -f $(TARGET)
