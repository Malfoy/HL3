CC=g++
CFLAGS= -Wall -Ofast -std=c++11  -flto -pipe -fopenmp -flto
LDFLAGS= -fopenmp  -flto



EXEC=bench_hlll



all: $(EXEC)



bench_hlll:   xxhash.o hlll.o
	$(CC) -o $@ $^ $(LDFLAGS)



hlll.o: hlll.cpp hlll.h
	$(CC) -o $@ -c $< $(CFLAGS)



xxhash.o: xxhash.c
	$(CC) -o $@ -c $< $(CFLAGS)



clean:
	rm -rf *.o
	rm -rf $(EXEC)


rebuild: clean $(EXEC)
