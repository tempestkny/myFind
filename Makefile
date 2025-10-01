CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2
TARGET = myfind

all: $(TARGET)

$(TARGET): myfind.o
	$(CXX) $(CXXFLAGS) -o $@ $^

myfind.o: myfind.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET)
