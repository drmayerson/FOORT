ifeq ($(default),1)
        CPU=1
        GPU=0
endif

CC = g++
CFLAGS = -std=c++17 -fopenmp -Ofast -Wno-unused-result
LDFLAGS = -lm -lconfig++ -lstdc++

SRC = Config.cpp Diagnostics.cpp Geodesic.cpp InputOutput.cpp Integrators.cpp Main.cpp Mesh.cpp Metric.cpp Terminations>OBJ = Config.o Diagnostics.o Geodesic.o InputOutput.o Integrators.o Main.o Mesh.o Metric.o Terminations.o ViewScreen.o

#all:
#       $(CC) -o FOORT $(CFLAGS) $(LDFLAGS)

harm: $(OBJ)
        $(CC) $(CFLAGS) -o FOORT $(OBJ) $(LDFLAGS)

%.o : %.cpp
        $(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

clean:
        rm *.o
        rm FOORT