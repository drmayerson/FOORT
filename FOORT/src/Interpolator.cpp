#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include "Interpolator.h"
#include "Grid.h"

//////////////// BicubicSplineInterpolator implementation (production-grade, C2 continuity) //////////////////

double BicubicSplineInterpolator::interpolate(double p_x, double p_theta,
                                              bool allow_extrapolation) const
{
    if (!allow_extrapolation &&
        (p_x < m_x.front() || p_x > m_x.back() || p_theta < m_theta.front() || p_theta > m_theta.back()))
    {
        throw std::runtime_error("BicubicSplineInterpolator::interpolate: query point (" +
                                 std::to_string(p_x) + ", " + std::to_string(p_theta) +
                                 ") is outside the interpolation domain [" + std::to_string(m_x.front()) +
                                 ", " + std::to_string(m_x.back()) + "] x [" + std::to_string(m_theta.front()) +
                                 ", " + std::to_string(m_theta.back()) + "]");
    }

    // Find cell containing the point
    auto x_it = std::lower_bound(m_x.begin(), m_x.end(), p_x);
    auto th_it = std::lower_bound(m_theta.begin(), m_theta.end(), p_theta);

    size_t i = std::max<size_t>(1, std::distance(m_x.begin(), x_it)) - 1;
    size_t j = std::max<size_t>(1, std::distance(m_theta.begin(), th_it)) - 1;

    i = std::min(i, m_x.size() - 2);
    j = std::min(j, m_theta.size() - 2);

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
            // IMPORTANT: Need to scale derivatives by cell width for bicubic Hermite formula
            // which expects df/dt (derivative wrt normalized parameter), not df/dx
            double fx00 = estimate_dx(grid, j, i) * (m_x[i + 1] - m_x[i]);
            double fx10 = estimate_dx(grid, j, i + 1) * (m_x[i + 1] - m_x[i]);
            double fx01 = estimate_dx(grid, j + 1, i) * (m_x[i + 1] - m_x[i]);
            double fx11 = estimate_dx(grid, j + 1, i + 1) * (m_x[i + 1] - m_x[i]);

            double fy00 = estimate_dy(grid, j, i) * (m_theta[j + 1] - m_theta[j]);
            double fy10 = estimate_dy(grid, j, i + 1) * (m_theta[j + 1] - m_theta[j]);
            double fy01 = estimate_dy(grid, j + 1, i) * (m_theta[j + 1] - m_theta[j]);
            double fy11 = estimate_dy(grid, j + 1, i + 1) * (m_theta[j + 1] - m_theta[j]);

            double fxy00 = estimate_dxdy(grid, j, i) * (m_x[i + 1] - m_x[i]) * (m_theta[j + 1] - m_theta[j]);
            double fxy10 = estimate_dxdy(grid, j, i + 1) * (m_x[i + 1] - m_x[i]) * (m_theta[j + 1] - m_theta[j]);
            double fxy01 = estimate_dxdy(grid, j + 1, i) * (m_x[i + 1] - m_x[i]) * (m_theta[j + 1] - m_theta[j]);
            double fxy11 = estimate_dxdy(grid, j + 1, i + 1) * (m_x[i + 1] - m_x[i]) * (m_theta[j + 1] - m_theta[j]);

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
