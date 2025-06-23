#include <fstream>
#include <iostream>

#ifndef GRID_H
#define GRID_H

//! A class for a 2D grid that contains the values of the necessary functions.
class Grid
{
public:
    //! Number of rows in the grid
    int N_row;
    //! Number of columns in the grid
    int N_col;
    //! Pointer to the data
    double *data;
    //! Number of rows times the number of columns
    int size;
    Grid(int N_row, int N_col);

    //! Destructor
    ~Grid() { delete[] this->data; }

    //! Overload the () operator to access the data
    double &operator()(int i, int j) { return this->data[i * N_col + j]; }

    void initialize_from_file(std::string file);
};

#endif // GRID_H
