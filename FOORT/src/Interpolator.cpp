#include <fstream>
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

// //! Interpolates the data of the grid
// //! @param grid The grid to be interpolated
// double Interpolator::interpolate(Grid *grid, double p_x, double p_theta)
// {
//     // find indices of theta and x
//     int x_ind = 0;
//     int th_ind = 0;
//     for (int k = 0; k < grid->N_col; ++k)
//     {
//         if (p_x <= m_x[k])
//         {
//             x_ind = k - 1;
//             break;
//         }
//     }
//     for (int k = 0; k < grid->N_row; ++k)
//     {
//         if (p_theta <= m_theta[k])
//         {
//             th_ind = k - 1;
//             break;
//         }
//     }

//     // find the interpolated value
//     double x1 = m_x[x_ind];
//     double x2 = m_x[x_ind + 1];
//     double y1 = m_theta[th_ind];
//     double y2 = m_theta[th_ind + 1];
//     double f11 = (*grid)(th_ind, x_ind);
//     double f12 = (*grid)(th_ind, x_ind + 1);
//     double f21 = (*grid)(th_ind + 1, x_ind);
//     double f22 = (*grid)(th_ind + 1, x_ind + 1);
//     double f = (f11 * (x2 - p_x) * (y2 - p_theta) +
//                 f12 * (p_x - x1) * (y2 - p_theta) +
//                 f21 * (x2 - p_x) * (p_theta - y1) +
//                 f22 * (p_x - x1) * (p_theta - y1)) /
//                ((x2 - x1) * (y2 - y1));
//     return f;
// }

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
