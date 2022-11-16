ifeq ($(default),1)
	CPU=1
	GPU=0
endif

CC = gcc
CFLAGS = -fopenmp -std=c99 -I/usr/include -Ofast -Wno-unused-result
LDFLAGS = -lm -lgsl -lcblas

SRC=Config.cpp Diagnostics.cpp Geodesic.cpp InputOutput.cpp Integrators.cpp Main.cpp Mesh.cpp Metric.cpp Terminations.cpp ViewScreen.cpp
OBJ=Config.o Diagnostics.o Geodesic.o InputOutput.o Integrators.o Main.o Mesh.o Metric.o Terminations.o ViewScreen.o


HDRHARM=raptor_harm_model.h

harm: $(OBJ) makefile
	$(CC) $(CFLAGS) -o FOORT $(OBJ) $(LDFLAGS)

clean:
	rm *.o
	rm FOORT
