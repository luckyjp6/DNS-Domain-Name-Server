all: main.cpp	
	g++ main.cpp -o dns -std=c++2a
clean: dns
	rm dns