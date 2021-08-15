CXX = g++
EIGEN_INC = /usr/include/eigen3
NLJS_INC = $(HOME)/opt/nljs/include

CXXFLAGS = -I $(EIGEN_INC) -I $(NLJS_INC) 
BATS = bats

testsrc = $(wildcard test_*.cpp)
testexe = $(patsubst test_%.cpp,bin/test_%, $(testsrc))
tstflag = $(patsubst test_%.cpp,test/test_%/okay, $(testsrc))

all: $(testexe) $(tstflag)

bin/test_%: test_%.cpp $(wildcard *.hpp)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $< -lboost_iostreams -lboost_filesystem

test/%/okay: bin/%
	$(BATS) -f $(notdir $<) test.bats && test -s $@

clean:
	rm -rf test bin
