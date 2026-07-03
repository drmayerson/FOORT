#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <string>

#include "Config.h"
#include "Diagnostics.h"
#include "Geodesic.h"
#include "Geometry.h"
#include "InputOutput.h"
#include "Integrators.h"
#include "Metric.h"
#include "Terminations.h"

namespace
{
    constexpr real kStableLightRingRadius = 15.47;
    constexpr real kUnstableLightRingRadius = 56.507305;

    constexpr int kNumX = 1500;
    constexpr int kNumTh = 200;
    constexpr real kL = 1.0;

    struct GeodesicResult
    {
        int step_count;
        real final_r;
        real closest_r;
    };

    std::string ResolveRotatingBosonStarDataFolder()
    {
        namespace fs = std::filesystem;

        const fs::path relative_data_folder = fs::path("RotatingBosonStar") / "data_C38";
        if (fs::exists(relative_data_folder))
        {
            return (relative_data_folder.string() + "/");
        }

        fs::path probe = fs::current_path();
        for (int i = 0; i < 8; ++i)
        {
            if (fs::exists(probe / relative_data_folder))
            {
                fs::current_path(probe);
                return (relative_data_folder.string() + "/");
            }

            const fs::path foort_root = probe / "FOORT";
            if (fs::exists(foort_root / relative_data_folder))
            {
                fs::current_path(foort_root);
                return (relative_data_folder.string() + "/");
            }

            if (!probe.has_parent_path())
            {
                break;
            }
            probe = probe.parent_path();
        }

        throw std::runtime_error("Could not locate RotatingBosonStar/data_C38.");
    }

    OneIndex RaiseIndex(const TwoIndex &guu, const OneIndex &covector)
    {
        OneIndex vector = {0.0, 0.0, 0.0, 0.0};
        for (int i = 0; i < dimension; ++i)
        {
            for (int j = 0; j < dimension; ++j)
            {
                vector[i] += guu[i][j] * covector[j];
            }
        }
        return vector;
    }

    void CheckMetricInvertibility(const TwoIndex &gdd, const TwoIndex &guu)
    {
        for (int a = 0; a < dimension; ++a)
        {
            for (int d = 0; d < dimension; ++d)
            {
                real sum = 0.0;
                for (int c = 0; c < dimension; ++c)
                {
                    sum += guu[a][c] * gdd[c][d];
                }

                if (a == d)
                {
                    EXPECT_NEAR(sum, 1.0, 1e-13)
                        << "Metric invertibility check failed at a = " << a << ", d = " << d;
                }
                else
                {
                    EXPECT_NEAR(sum, 0.0, 1e-13)
                        << "Metric invertibility check failed at a = " << a << ", d = " << d;
                }
            }
        }
    }

    real NullAngularMomentumAtRadius(const Metric *metric, real radius, real energy)
    {
        Point orbit_pos = {0.0, radius, pi / 2.0, 0.0};
        const TwoIndex guu = metric->getMetric_uu(orbit_pos);

        const real gu_tt = guu[0][0];
        const real gu_tphi = guu[0][3];
        const real gu_phiphi = guu[3][3];

        const real discriminant = gu_tphi * gu_tphi * energy * energy - gu_tt * gu_phiphi * energy * energy;
        if (!std::isfinite(discriminant) || discriminant <= 0.0 || gu_phiphi == 0.0)
        {
            throw std::runtime_error("Failed to compute null angular momentum for RotatingBosonStarMetric.");
        }

        return (gu_tphi * energy - std::sqrt(discriminant)) / gu_phiphi;
    }

    GeodesicResult IntegrateRotatingBosonStarLightRing(real angular_momentum_radius,
                                                       real start_radius,
                                                       const std::string &output_filename)
    {
        const bool rLogScale = false;
        const std::string metric_folder = ResolveRotatingBosonStarDataFolder();

        std::unique_ptr<Metric> theM = std::unique_ptr<Metric>(
            new RotatingBosonStarMetric(rLogScale, metric_folder, kNumX, 
                kNumTh, kL, true));
        std::unique_ptr<Source> theS = std::unique_ptr<Source>(new NoSource(theM.get()));

        GeodesicPositionDiagnostic::DiagOptions =
            std::unique_ptr<GeodesicPositionOptions>(
                new GeodesicPositionOptions{0, UpdateFrequency{1, false, false}});

        ClosestRadiusDiagnostic::DiagOptions =
            std::unique_ptr<ClosestRadiusOptions>(
                new ClosestRadiusOptions{rLogScale, UpdateFrequency{1, false, false}});

        const DiagBitflag all_diags = Diag_GeodesicPosition | Diag_ClosestRadius;
        const DiagBitflag val_diag = Diag_ClosestRadius;

        const TermBitflag all_terms = Term_BoundarySphere | Term_TimeOut | Term_NaN;
        BoundarySphereTermination::TermOptions =
            std::unique_ptr<BoundarySphereTermOptions>(
                new BoundarySphereTermOptions{120.0, rLogScale, 1});
        TimeOutTermination::TermOptions =
            std::unique_ptr<TimeOutTermOptions>(
                new TimeOutTermOptions{50000, 1});
        NaNTermination::TermOptions =
            std::unique_ptr<NaNTermOptions>(
                new NaNTermOptions{true, 1});

        GeodesicIntegratorFunc theIntegrator = Integrators::IntegrateGeodesicStep_RK4;
        Integrators::IntegratorDescription = "RK4";
        Integrators::epsilon = 0.01;

        Geodesic theGeod(theM.get(), theS.get(), all_diags, val_diag, all_terms, theIntegrator);

        const real energy = 1.0;
        const real angular_momentum = NullAngularMomentumAtRadius(theM.get(), angular_momentum_radius, energy);
        const OneIndex initvel_d = {-energy, 0.0, 0.0, angular_momentum};
        std::cout << "Integrating geodesic with angular momentum radius " << angular_momentum_radius
                  << " and corresponding null angular momentum " << angular_momentum << ".\n";

        const Point initpos = {0.0, rLogScale ? log(start_radius) : start_radius, pi / 2.0, 0.0};
        const TwoIndex gdd_at_start = theM->getMetric_dd(initpos);
        const TwoIndex guu_at_start = theM->getMetric_uu(initpos);
        CheckMetricInvertibility(gdd_at_start, guu_at_start);
        const OneIndex initvel = RaiseIndex(guu_at_start, initvel_d);

        ScreenIndex scrindex = {0, 0};
        theGeod.Reset(scrindex, initpos, initvel);

        std::ofstream output(output_filename);
        output << std::setprecision(15);
        output << "# t r theta phi\n";

        Point current_pos = initpos;
        int step_count = 0;

        while (theGeod.getTermCondition() == Term::Continue)
        {
            theGeod.Update();
            current_pos = theGeod.getCurrentPos();

            const TwoIndex gdd = theM->getMetric_dd(current_pos);
            const TwoIndex guu = theM->getMetric_uu(current_pos);
            CheckMetricInvertibility(gdd, guu);

            const real current_r = rLogScale ? exp(current_pos[1]) : current_pos[1];
            output << current_pos[0] << " " << current_r << " "
                   << current_pos[2] << " " << current_pos[3] << "\n";
            ++step_count;
        }

        output.close();

        const real final_r = rLogScale ? exp(current_pos[1]) : current_pos[1];
        const real closest_r = rLogScale ? exp(theGeod.getDiagnosticFinalValue()[0]) : theGeod.getDiagnosticFinalValue()[0];

        return GeodesicResult{step_count, final_r, closest_r};
    }
} // namespace

TEST(RotatingBosonStar, stable_light_ring)
{
    const GeodesicResult result = IntegrateRotatingBosonStarLightRing(
        kStableLightRingRadius,
        kStableLightRingRadius,
        "rotating_boson_star_stable_light_ring.dat");

    EXPECT_GT(result.step_count, 0) << "Geodesic should integrate for at least one step.";
    EXPECT_TRUE(std::isfinite(result.final_r)) << "Final radius should be finite.";
    EXPECT_TRUE(std::isfinite(result.closest_r)) << "Closest radius diagnostic should be finite.";

    EXPECT_NEAR(result.closest_r, kStableLightRingRadius, 0.3)
        << "Stable light ring closest radius should stay near 15.5.";
    EXPECT_NEAR(result.final_r, kStableLightRingRadius, 0.6)
        << "Stable light ring final radius should remain near 15.5.";
}

TEST(RotatingBosonStar, unstable_light_ring)
{
    constexpr real perturbation = 1e-6;
    const GeodesicResult result = IntegrateRotatingBosonStarLightRing(
        kUnstableLightRingRadius,
        kUnstableLightRingRadius + perturbation,
        "rotating_boson_star_unstable_light_ring.dat");

    EXPECT_GT(result.step_count, 0) << "Geodesic should integrate for at least one step.";
    EXPECT_TRUE(std::isfinite(result.final_r)) << "Final radius should be finite.";
    EXPECT_TRUE(std::isfinite(result.closest_r)) << "Closest radius diagnostic should be finite.";

    EXPECT_GT(std::abs(result.final_r - kUnstableLightRingRadius), 0.1)
        << "Unstable light ring trajectory should depart from radius 56.";
    EXPECT_GT(std::abs(result.final_r - (kUnstableLightRingRadius + perturbation)), 0.05)
        << "Trajectory should move away from the perturbed starting radius.";
}
