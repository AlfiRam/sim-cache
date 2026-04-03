CC = g++
OPT = -O3
WARN = -Wall
CFLAGS = $(OPT) $(WARN)

SIM_SRC = sim_cache.cc cache.cc trace_preprocessor.cc

SIM_OBJ = sim_cache.o cache.o trace_preprocessor.o

all: sim_cache

sim_cache: $(SIM_OBJ)
	$(CC) -o sim_cache $(CFLAGS) $(SIM_OBJ) -lm

.cc.o:
	$(CC) $(CFLAGS) -c $*.cc

clean:
	rm -f *.o sim_cache

clobber:
	rm -f *.o
