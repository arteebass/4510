# Makefile
all: proxy
	
proxy : proxy.cpp proxy.h
	g++ -Wall -pthread proxy.cpp -o MyProxy
	
clean:
	rm -f MyProxy