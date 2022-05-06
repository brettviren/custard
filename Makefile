CXX = g++
EIGEN_INC = /usr/include/eigen3
NLJS_INC = $(HOME)/opt/nljs/include

CXXFLAGS = -std=c++17 -I $(EIGEN_INC) -I $(NLJS_INC) -Wpedantic -Wall 
CXXFLAGS += -O3 
BATS = bats

testsrc = $(wildcard test_*.cpp)
testexe = $(patsubst test_%.cpp,bin/test_%, $(testsrc))
# tstflag = $(patsubst test_%.cpp,test/test_%/okay, $(testsrc))


all: $(testexe) $(tstflag)


bin/test_%: test_%.cpp miniz.c
	mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $< miniz.c -lboost_iostreams -lboost_filesystem -lboost_system  -lz

# test/%/okay: bin/%
# 	$(BATS) -f $(notdir $<) test.bats && test -s $@

clean:
	rm -rf test bin *.d

%.d: %.cpp
	@set -e; rm -f $@; \
	$(CXX) -M $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,bin/\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
include $(testsrc:.cpp=.d)
