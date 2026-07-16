#include <fstream>
#include <iostream>
#include <sstream>
#include "Grid.h"

//! A class for a 2D grid that contains the values of the necessary functions.
Grid::Grid(int N_row, int N_col)
{
    this->N_row = N_row;
    this->N_col = N_col;
    this->data = new double[N_row * N_col];
    size = N_row * N_col;
}

void Grid::initialize_from_file(std::string file)
{
    std::ifstream inputFile(file);

    if (!inputFile)
    {
        std::cerr << "Error opening file!" << std::endl;
    }

    std::string line;
    int i = 0;
    int j = 0;

    while (std::getline(inputFile, line))
    {
        std::istringstream iss(line);

        double val;
        while (iss >> val)
        {
            data[i * N_col + j] = val;
            j += 1;
        }
        j = 0;
        i += 1;
    }
    std::cout << "Grid initialized from " << file << std::endl
              << std::endl;

    // close the file
    inputFile.close();
}
