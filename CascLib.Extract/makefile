all: casc lookup3.o clean

lookup3.o: ../CascLib/lookup3.c
	clang-3.6 -c ../CascLib/lookup3.c -o lookup3.o

casc: main.cpp lookup3.o
	clang++-3.6 -std=c++14 -ggdb -o casc main.cpp lookup3.o -I../CascLib/include -lz -lboost_filesystem -lboost_system

clean:
	rm lookup3.o
