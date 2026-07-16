#include <gtest/gtest.h>
#include <fstream>
#include <iomanip>
#include <cmath>

#include "Metric.h" // Metrics
#include "Geometry.h"

/**
 * @brief Test interpolators in RotatingBosonStar metric
 * @details Tests f, l, g, and omega interpolators at various x and theta values
 */
TEST(RotatingBosonStarInterpolators, test_interpolators_at_various_points)
{
    // Setup metric with same parameters as main test
    bool rLogScale = true;
    std::string MetricFolder = "RotatingBosonStar/data_C38/";
    int NumX = 1500;
    int NumTh = 200;
    real L = 1.0;

    std::unique_ptr<Metric> theM = std::unique_ptr<Metric>(
        new RotatingBosonStarMetric(rLogScale, MetricFolder, NumX, NumTh, L));

    auto metric = static_cast<RotatingBosonStarMetric *>(theM.get());

    // Open output file to log results
    std::ofstream outfile("interpolator_test_results.txt");
    outfile << std::scientific << std::setprecision(10);
    outfile << "Testing RotatingBosonStarMetric interpolators\n";
    outfile << "MetricFolder: " << MetricFolder << "\n";
    outfile << "NumX: " << NumX << ", NumTh: " << NumTh << ", L: " << L << "\n";
    outfile << "rLogScale: " << (rLogScale ? "true" : "false") << "\n\n";

    // Test points: various x and theta values
    std::vector<real> x_values = {0.001, 0.01, 0.1, 0.3, 0.5, 0.7, 0.9, 0.95, 0.99};
    std::vector<real> theta_values = {0.01, 0.1, M_PI / 4.0, M_PI / 2.0, 3.0 * M_PI / 4.0, M_PI - 0.1, M_PI - 0.01};

    outfile << "x values to test: ";
    for (auto x : x_values)
        outfile << x << " ";
    outfile << "\n";
    outfile << "theta values to test: ";
    for (auto th : theta_values)
        outfile << th << " ";
    outfile << "\n\n";

    outfile << std::left << std::setw(12) << "x"
            << std::setw(12) << "theta"
            << std::setw(15) << "f"
            << std::setw(15) << "l"
            << std::setw(15) << "g"
            << std::setw(15) << "omega\n";
    outfile << std::string(69, '-') << "\n";

    int nan_count = 0;
    int total_count = 0;

    for (auto x : x_values)
    {
        for (auto theta : theta_values)
        {
            try
            {
                // Get interpolated values
                real f_val = metric->m_fInterpolator->interpolate(x, theta);
                real l_val = metric->m_lInterpolator->interpolate(x, theta);
                real g_val = metric->m_gInterpolator->interpolate(x, theta);
                real omega_val = metric->m_OmegaInterpolator->interpolate(x, theta);

                total_count++;

                // Check for NaN
                bool has_nan = std::isnan(f_val) || std::isnan(l_val) ||
                               std::isnan(g_val) || std::isnan(omega_val);

                if (has_nan)
                    nan_count++;

                // Output results
                outfile << std::setw(12) << x
                        << std::setw(12) << theta
                        << std::setw(15) << f_val
                        << std::setw(15) << l_val
                        << std::setw(15) << g_val
                        << std::setw(15) << omega_val;

                if (has_nan)
                    outfile << " <- NAN";
                outfile << "\n";
            }
            catch (const std::exception &e)
            {
                total_count++;
                outfile << std::setw(12) << x
                        << std::setw(12) << theta
                        << "ERROR: " << e.what() << "\n";
            }
        }
    }

    outfile << "\n"
            << std::string(69, '=') << "\n";
    outfile << "Summary:\n";
    outfile << "Total test points: " << total_count << "\n";
    outfile << "Points with NaN: " << nan_count << "\n";
    outfile << "Success rate: " << (total_count - nan_count) << "/" << total_count << "\n";

    outfile.close();

    // Special test for specific geodesic coordinates
    double test_x = 0.998752;
    double test_theta = 1.57;

    real f_test = metric->m_fInterpolator->interpolate(test_x, test_theta);
    real l_test = metric->m_lInterpolator->interpolate(test_x, test_theta);
    real g_test = metric->m_gInterpolator->interpolate(test_x, test_theta);
    real omega_test = metric->m_OmegaInterpolator->interpolate(test_x, test_theta);

    // Basic assertions
    ASSERT_GT(total_count, 0) << "Should have tested at least one point";
    ASSERT_EQ(nan_count, 0) << "Should not have NaN values in interpolations";
    ASSERT_TRUE(std::isfinite(f_test));
    ASSERT_TRUE(std::isfinite(l_test));
    ASSERT_TRUE(std::isfinite(g_test));
    ASSERT_TRUE(std::isfinite(omega_test));
}
