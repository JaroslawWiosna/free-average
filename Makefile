
.PHONY: all
all: free-average

free-average: main.cpp Makefile
	g++ -std=c++17 -o free-average main.cpp

install:
	echo "TODO: make install"

.PHONY: clean
clean:
	rm -rf free-average
