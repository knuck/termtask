CC = g++ -std=c++0x
X11 = -lX11
all: main strip

main: main.cpp
	$(CC) -Os main.cpp $(X11)
debug: main.cpp
	$(CC) -g -O0 main.cpp $(X11)
strip:
	strip ./a.out
clean:
	rm -rf *.o
