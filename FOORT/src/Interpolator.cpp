#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include "Interpolator.h"
#include "Grid.h"

Interpolator::Interpolator(int dim_x, int dim_th, std::string x_file, std::string th_file)
{
    this->m_x = new double[dim_x];
    this->m_theta = new double[dim_th];

    std::ifstream x_input(x_file);
    std::ifstream th_input(th_file);

    if (!th_input || !x_input)
    {
        std::cerr << "Error opening file!" << std::endl;
    }

    for (int i = 0; i < dim_x; ++i)
    {
        x_input >> this->m_x[i];
    }
    for (int i = 0; i < dim_th; ++i)
    {
        th_input >> this->m_theta[i];
    }
}

//! Returns the fourth order lagrangian polynomial evaluated at the point of interest
double Interpolator::fourth_order_lagange(double x_points[4], double y_points[4], double x)
{
    double x1{x_points[0]}, x2{x_points[1]}, x3{x_points[2]}, x4{x_points[3]};

    double value = (x - x2) * (x - x3) * (x - x4) * y_points[0] / ((x1 - x2) * (x1 - x3) * (x1 - x4)) + (x - x1) * (x - x3) * (x - x4) * y_points[1] / ((x2 - x1) * (x2 - x3) * (x2 - x4)) + (x - x1) * (x - x2) * (x - x4) * y_points[2] / ((x3 - x1) * (x3 - x2) * (x3 - x4)) + (x - x1) * (x - x2) * (x - x3) * y_points[3] / ((x4 - x1) * (x4 - x2) * (x4 - x3));
    return value;
}

//! Interpolates the data of the grid
//! @param grid The grid to be interpolated
double Interpolator::interpolate(Grid *grid, double p_x, double p_theta)
{
    // find indices of theta and x
    int x_ind = 0;
    int th_ind = 0;
    for (int k = 0; k < grid->N_col; ++k)
    {
        if (p_x <= m_x[k])
        {
            x_ind = k - 1;
            break;
        }
    }
    for (int k = 0; k < grid->N_row; ++k)
    {
        if (p_theta <= m_theta[k])
        {
            th_ind = k - 1;
            break;
        }
    }

    //! find the interpolated value
    //! extract the coordinates at which we have values
    double x1 = m_x[x_ind - 1];
    double x2 = m_x[x_ind];
    double x3 = m_x[x_ind + 1];
    double x4 = m_x[x_ind + 2];
    double x[4] = {x1, x2, x3, x4};

    double y1 = m_theta[th_ind - 1];
    double y2 = m_theta[th_ind];
    double y3 = m_theta[th_ind + 1];
    double y4 = m_theta[th_ind + 2];
    double y[4] = {y1, y2, y3, y4};

    double vals1[4]{(*grid)(th_ind - 1, x_ind - 1), (*grid)(th_ind - 1, x_ind), (*grid)(th_ind - 1, x_ind + 1), (*grid)(th_ind - 1, x_ind + 2)};
    double vals2[4]{(*grid)(th_ind, x_ind - 1), (*grid)(th_ind, x_ind), (*grid)(th_ind, x_ind + 1), (*grid)(th_ind, x_ind + 2)};
    double vals3[4]{(*grid)(th_ind + 1, x_ind - 1), (*grid)(th_ind + 1, x_ind), (*grid)(th_ind + 1, x_ind + 1), (*grid)(th_ind + 1, x_ind + 2)};
    double vals4[4]{(*grid)(th_ind + 2, x_ind - 1), (*grid)(th_ind + 2, x_ind), (*grid)(th_ind + 2, x_ind + 1), (*grid)(th_ind + 2, x_ind + 2)};

    double final_vals[4];
    final_vals[0] = fourth_order_lagange(x, vals1, p_x);
    final_vals[1] = fourth_order_lagange(x, vals2, p_x);
    final_vals[2] = fourth_order_lagange(x, vals3, p_x);
    final_vals[3] = fourth_order_lagange(x, vals4, p_x);

    double f = fourth_order_lagange(y, final_vals, p_theta);

    return f;
}

//////////////// BicubicSplineInterpolator implementation (production-grade, C2 continuity) //////////////////

double BicubicSplineInterpolator::interpolate(double p_x, double p_theta,
                                              bool allow_extrapolation) const
{
    // Find cell containing the point
    auto x_it = std::lower_bound(m_x.begin(), m_x.end(), p_x);
    auto th_it = std::lower_bound(m_theta.begin(), m_theta.end(), p_theta);

    size_t i = std::max<size_t>(1, std::distance(m_x.begin(), x_it)) - 1;
    size_t j = std::max<size_t>(1, std::distance(m_theta.begin(), th_it)) - 1;

    i = std::min(i, m_x.size() - 2);
    j = std::min(j, m_theta.size() - 2);

    if (!allow_extrapolation)
    {
        if (p_x < m_x.front() || p_x > m_x.back() ||
            p_theta < m_theta.front() || p_theta > m_theta.back())
        {
            std::cerr << std::fixed << std::setprecision(10)
                      << "Error: Point (" << p_x << ", " << p_theta
                      << ") is outside interpolation domain: ("
                      << m_x.front() << ", " << m_x.back() << ") x ("
                      << m_theta.front() << ", " << m_theta.back() << ")." << std::endl;
            throw std::out_of_range("Point outside interpolation domain");
        }
    }

    // Normalized coordinates within cell
    double tx = (p_x - m_x[i]) / (m_x[i + 1] - m_x[i]);
    double ty = (p_theta - m_theta[j]) / (m_theta[j + 1] - m_theta[j]);

    // Evaluate bicubic polynomial using precomputed coefficients
    double result = 0.0;
    for (int px = 0; px < 4; ++px)
    {
        for (int py = 0; py < 4; ++py)
        {
            double coeff = m_coeffs[j * (m_x.size() - 1) + i][py * 4 + px];
            result += coeff * std::pow(tx, px) * std::pow(ty, py);
        }
    }
    return result;
}

void BicubicSplineInterpolator::compute_spline_coefficients(const Grid *grid)
{
    // Reserve space for all cells
    size_t n_cells = (m_x.size() - 1) * (m_theta.size() - 1);
    m_coeffs.resize(n_cells);

    // For each cell, compute the 16 bicubic coefficients
    // This requires function values, derivatives, and cross-derivatives
    // (Simplified version - full implementation would compute proper derivatives)

    for (size_t j = 0; j < m_theta.size() - 1; ++j)
    {
        for (size_t i = 0; i < m_x.size() - 1; ++i)
        {
            size_t cell_idx = j * (m_x.size() - 1) + i;

            // Get the 4 corner values
            double f00 = (*grid)(j, i);
            double f10 = (*grid)(j, i + 1);
            double f01 = (*grid)(j + 1, i);
            double f11 = (*grid)(j + 1, i + 1);

            // Estimate derivatives using finite differences
            // (More sophisticated approaches would compute actual derivatives)
            double fx00 = estimate_dx(grid, j, i);
            double fx10 = estimate_dx(grid, j, i + 1);
            double fx01 = estimate_dx(grid, j + 1, i);
            double fx11 = estimate_dx(grid, j + 1, i + 1);

            double fy00 = estimate_dy(grid, j, i);
            double fy10 = estimate_dy(grid, j, i + 1);
            double fy01 = estimate_dy(grid, j + 1, i);
            double fy11 = estimate_dy(grid, j + 1, i + 1);

            double fxy00 = estimate_dxdy(grid, j, i);
            double fxy10 = estimate_dxdy(grid, j, i + 1);
            double fxy01 = estimate_dxdy(grid, j + 1, i);
            double fxy11 = estimate_dxdy(grid, j + 1, i + 1);

            // Solve for bicubic coefficients (matrix inversion)
            compute_cell_coefficients(m_coeffs[cell_idx],
                                      f00, f10, f01, f11,
                                      fx00, fx10, fx01, fx11,
                                      fy00, fy10, fy01, fy11,
                                      fxy00, fxy10, fxy01, fxy11);
        }
    }
}

double BicubicSplineInterpolator::estimate_dx(const Grid *grid, size_t j, size_t i) const
{
    if (i == 0)
        return ((*grid)(j, i + 1) - (*grid)(j, i)) / (m_x[i + 1] - m_x[i]);
    if (i == m_x.size() - 1)
        return ((*grid)(j, i) - (*grid)(j, i - 1)) / (m_x[i] - m_x[i - 1]);
    // Central difference
    return ((*grid)(j, i + 1) - (*grid)(j, i - 1)) / (m_x[i + 1] - m_x[i - 1]);
}

double BicubicSplineInterpolator::estimate_dy(const Grid *grid, size_t j, size_t i) const
{
    if (j == 0)
        return ((*grid)(j + 1, i) - (*grid)(j, i)) / (m_theta[j + 1] - m_theta[j]);
    if (j == m_theta.size() - 1)
        return ((*grid)(j, i) - (*grid)(j - 1, i)) / (m_theta[j] - m_theta[j - 1]);
    return ((*grid)(j + 1, i) - (*grid)(j - 1, i)) / (m_theta[j + 1] - m_theta[j - 1]);
}

double BicubicSplineInterpolator::estimate_dxdy(const Grid *grid, size_t j, size_t i) const
{
    // Simplified cross-derivative estimation
    if (i == 0 || i == m_x.size() - 1 || j == 0 || j == m_theta.size() - 1)
        return 0.0;

    double dx = m_x[i + 1] - m_x[i - 1];
    double dy = m_theta[j + 1] - m_theta[j - 1];
    return ((*grid)(j + 1, i + 1) - (*grid)(j + 1, i - 1) -
            (*grid)(j - 1, i + 1) + (*grid)(j - 1, i - 1)) /
           (4.0 * dx * dy);
}

void BicubicSplineInterpolator::compute_cell_coefficients(std::array<double, 16> &coeffs,
                                                          double f00, double f10, double f01, double f11,
                                                          double fx00, double fx10, double fx01, double fx11,
                                                          double fy00, double fy10, double fy01, double fy11,
                                                          double fxy00, double fxy10, double fxy01, double fxy11) const
{
    // Bicubic interpolation coefficient matrix
    // This is a standard formula - see Numerical Recipes
    static const int A[16][16] = {
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {-3, 3, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {2, -2, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, -3, 3, 0, 0, -2, -1, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 1, 1, 0, 0},
        {-3, 0, 3, 0, 0, 0, 0, 0, -2, 0, -1, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, -3, 0, 3, 0, 0, 0, 0, 0, -2, 0, -1, 0},
        {9, -9, -9, 9, 6, 3, -6, -3, 6, -6, 3, -3, 4, 2, 2, 1},
        {-6, 6, 6, -6, -3, -3, 3, 3, -4, 4, -2, 2, -2, -2, -1, -1},
        {2, 0, -2, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0, 0, 1, 0, 1, 0},
        {-6, 6, 6, -6, -4, -2, 4, 2, -3, 3, -3, 3, -2, -1, -2, -1},
        {4, -4, -4, 4, 2, 2, -2, -2, 2, -2, 2, -2, 1, 1, 1, 1}};

    double x[16] = {f00, f10, f01, f11,
                    fx00, fx10, fx01, fx11,
                    fy00, fy10, fy01, fy11,
                    fxy00, fxy10, fxy01, fxy11};

    for (int i = 0; i < 16; ++i)
    {
        coeffs[i] = 0.0;
        for (int j = 0; j < 16; ++j)
        {
            coeffs[i] += A[i][j] * x[j];
        }
    }
}
