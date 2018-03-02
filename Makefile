all: mkdirs ina219.o simple-auto-gain.o auto-gain-high-resolution.o manual-gain-high-resolution.o

mkdirs:
	mkdir ./build
	mkdir ./build/lib
	mkdir ./build/examples

simple-auto-gain.o: ina219.o
	g++ -o ./build/lib/simple-auto-gain.o -c ./examples/simple-auto-gain.c
	g++ -o ./build/examples/simple-auto-gain ./build/lib/simple-auto-gain.o ./build/lib/ina219.o

auto-gain-high-resolution.o: ina219.o
	g++ -o ./build/lib/auto-gain-high-resolution.o -c ./examples/auto-gain-high-resolution.c
	g++ -o ./build/examples/auto-gain-high-resolution ./build/lib/auto-gain-high-resolution.o ./build/lib/ina219.o

manual-gain-high-resolution.o: ina219.o
	g++ -o ./build/lib/manual-gain-high-resolution.o -c ./examples/manual-gain-high-resolution.c
	g++ -o ./build/examples/manual-gain-high-resolution ./build/lib/manual-gain-high-resolution.o ./build/lib/ina219.o

ina219.o:
	g++ -o ./build/lib/ina219.o -c ina219.cc

clean:
	rm -rf ./build/*.o
	rm -rf ./build/lib/*.o
	rm -rf ./build/examples/*