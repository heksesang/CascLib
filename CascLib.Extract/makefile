all: casc

casc: main.cpp
	clang++-3.6 -std=c++1z -ggdb -o casc main.cpp -I../CascLib/include -lz -lboost_filesystem -lboost_system

clean:
	rm casc