CXX = g++
#CXXFLAGS = -O2 -DDEBUG -std=c++17 -Wall -pedantic -fanalyzer -ggdb
#CXXFLAGS = -O2 -DDEBUG -std=c++17 -Wall -pedantic -ggdb
CXXFLAGS = -O2 -std=c++17 -Wall -pedantic
#CXXFLAGS = -O0 -std=c++17 -Wall -pedantic -ggdb
COMMON_H = tm.h parser.h utils.h
COMMON_S = tm.cpp parser.cpp utils.cpp
COMMON_O = tm.o parser.o utils.o

all: turing

test: tests.sh turing
	./tests.sh

utils.o: utils.h utils.cpp
	$(CXX) $(CXXFLAGS) -c utils.cpp

tm.o: utils.h tm.h tm.cpp
	$(CXX) $(CXXFLAGS) -c tm.cpp

parser.o: utils.h tm.h parser.h parser.cpp
	$(CXX) $(CXXFLAGS) -c parser.cpp

turing.o: turing.cpp $(COMMON_H)
	$(CXX) $(CXXFLAGS) -c turing.cpp

turing: turing.o $(COMMON_H) $(COMMON_O)
	$(CXX) $(CXXFLAGS) -o $@ $@.o $(COMMON_O)

clean:
	rm -f turing *.o
