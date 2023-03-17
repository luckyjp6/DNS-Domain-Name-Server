all: main.cpp	
	g++ main.cpp -o dns -std=c++2a
	./dns 10001 ./config config.txt