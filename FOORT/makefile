CC = g++
CFLAGS = -std=c++17 -fopenmp -O3 -Wno-unused-result
LDFLAGS = -lm -lstdc++fs


SRC = Config.cpp ConfigReader.cpp Diagnostics.cpp Diagnostics_Emission.cpp Geodesic.cpp InputOutput.cpp Integrators.cpp Main.cpp Mesh.cpp Metric.cpp Terminations.cpp Utilities.cpp ViewScreen.cpp 
OBJ = Config.o ConfigReader.o Diagnostics.o Diagnostics_Emission.o Geodesic.o InputOutput.o Integrators.o Main.o Mesh.o Metric.o Terminations.o Utilities.o ViewScreen.o

harm: $(OBJ)
	$(CC) $(CFLAGS) -o FOORT $(OBJ) $(LDFLAGS)

%.o : %.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

clean:
	rm *.o
	rm FOORT