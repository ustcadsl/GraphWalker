INCFLAGS = -I/usr/local/include/ -I./src/

CPP = g++
CPPFLAGS = -g -O0 $(INCFLAGS)  -fopenmp -Wall -Wno-strict-aliasing 
# CPPFLAGS = -g -O3 $(INCFLAGS)  -fopenmp -Wall -Wno-strict-aliasing 
LINKERFLAGS = -lz
DEBUGFLAGS = -g -ggdb $(INCFLAGS)
HEADERS=$(shell find . -name '*.hpp')


apps : apps/rwdomination apps/graphlet apps/simrank apps/msppr apps/rawrandomwalks
 
echo:
	echo $(HEADERS)
clean:
	@rm -rf bin/*

apps/% : apps/%.cpp $(HEADERS)
	@mkdir -p bin/$(@D)
	$(CPP) $(CPPFLAGS) -Iapp/ $@.cpp -o bin/$@ $(LINKERFLAGS)

# apps/% : apps/%.cpp $(HEADERS) bin/cudacode.o
# 	@mkdir -p bin/$(@D)
# 	$(CPP) $(CPPFLAGS) -Iapp/ $@.cpp bin/cudacode.o -l/usr/local/cuda/lib64/libcudart.so -o bin/$@ $(LINKERFLAGS)

# bin/cudacode.o:
# 	nvcc -I./src/ -c src/cuda/exec_update.cu

# test pagerank
testp:
	make apps/pagerank
	./bin/apps/pagerank file "/home/wang/Documents/DataSet/Wikipedia/wikipedia_sorted.data" nvertices 12150977 nedges 378102402 nshards 5 R 10 L 10 tail 0

# test personalizedpagerank
testpp:
	make apps/personalizedpagerank
	./bin/apps/personalizedpagerank
