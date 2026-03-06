#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <stdexcept>
#include <algorithm>
#include <cmath>
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

class BicubicSplineInterpolator
{
public:
    BicubicSplineInterpolator(const std::vector<double> &x_coords,
                              const std::vector<double> &theta_coords,
                              const Grid *grid)
        : m_x(x_coords), m_theta(theta_coords)
    {
        // Precompute spline coefficients for better performance
        // This is more complex but provides C2 continuity
        compute_spline_coefficients(grid);
    }

    double interpolate(double p_x, double p_theta,
                       bool allow_extrapolation = false) const;

private:
    std::vector<double> m_x;
    std::vector<double> m_theta;
    std::vector<std::array<double, 16>> m_coeffs; // 16 coefficients per cell

    void compute_spline_coefficients(const Grid *grid);
    double estimate_dx(const Grid *grid, size_t j, size_t i) const;
    double estimate_dy(const Grid *grid, size_t j, size_t i) const;
    double estimate_dxdy(const Grid *grid, size_t j, size_t i) const;
    void compute_cell_coefficients(std::array<double, 16> &coeffs,
                                   double f00, double f10, double f01, double f11,
                                   double fx00, double fx10, double fx01, double fx11,
                                   double fy00, double fy10, double fy01, double fy11,
                                   double fxy00, double fxy10, double fxy01, double fxy11) const;
};

#endif // INTERPOLATOR_H
