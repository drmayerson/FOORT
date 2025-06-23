#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "Grid.h"

#ifndef INTERPOLATOR_H
#define INTERPOLATOR_H

class Interpolator
{
public:
    double *m_x;     // Pointer to the x values
    double *m_theta; // Pointer to the theta values

    //! Constructor
    Interpolator(int dim_x, int dim_th, std::string x_file = "RotatingBosonStar/x.txt", std::string th_file = "RotatingBosonStar/theta.txt");

    //! Destructor
    ~Interpolator()
    {
        delete[] this->m_x;
        delete[] this->m_theta;
    }

    //! Interpolates the data of the grid
    //! @param grid The grid to be interpolated
    double interpolate(Grid *grid, double p_x, double p_theta);

private:
    //! Returns the fourth order lagrangian polynomial evaluated at the point of interest
    double fourth_order_lagange(double x_points[4], double y_points[4], double x);
};

#endif // INTERPOLATOR_H
