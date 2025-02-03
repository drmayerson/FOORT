#include <gtest/gtest.h>
#include <iostream>
#include <fstream>

#include "Config.h"
#include "Geometry.h"
#include "Metric.h"       // Metrics
#include "Diagnostics.h"  // Diagnostics
#include "Terminations.h" // Termination conditions
#include "Geodesic.h"     // Geodesics (and Sources)
#include "Integrators.h"  // Integrator functions
#include "InputOutput.h"  // Output to screen and files

//! Test the RK4 integration of the bound circular geodesic at r=3M, which in theory should remain at r=3M.
//! The test fails if the final radial coordinate differs by 1e-3 from 3M after 5 half orbits, i.e. after the azimuthal
//! coordinate changed by 5 pi OR if the theta polar coordinate changed by more than 1e-9.
TEST(Integrators, rk4_einstein_ring)
{
    std::unique_ptr<Metric> theM = std::unique_ptr<Metric>(new KerrMetric(0., false, 1.));
    std::unique_ptr<Source> theS = std::unique_ptr<Source>(new NoSource(theM.get()));

    ClosestRadiusDiagnostic::DiagOptions =
        std::unique_ptr<ClosestRadiusOptions>(new ClosestRadiusOptions{false, UpdateFrequency{1, false, false}});

    DiagBitflag AllDiags = Diag_ClosestRadius;
    DiagBitflag ValDiag = Diag_ClosestRadius;

    TermBitflag AllTerms = Term_BoundarySphere;

    BoundarySphereTermination::TermOptions =
        std::unique_ptr<BoundarySphereTermOptions>(new BoundarySphereTermOptions{20., false, 1});

    GeodesicIntegratorFunc theIntegrator = Integrators::IntegrateGeodesicStep_RK4;
    Integrators::IntegratorDescription = "RK4";
    Integrators::epsilon = 0.02;

    Geodesic theGeod(theM.get(), theS.get(), AllDiags, ValDiag, AllTerms,
                     theIntegrator);

    // Set up initial conditions for the bound geodesic
    Point initpos = {0.0, 20.0, pi / 2.0, 0.0};
    double xi = 0.24904964;

    TwoIndex gdd = theM->getMetric_dd(initpos);

    OneIndex initvel = {1.0 / sqrt(-gdd[0][0]), -cos(xi) / sqrt(gdd[1][1]), 0.0, sin(xi) / sqrt(gdd[3][3])};
    ScreenIndex scrindex = {0, 0};

    theGeod.Reset(scrindex, initpos, initvel);

    Point current_pos{initpos};
    double current_t{0.0}, current_phi{0.0}, current_r{20.0};
    std::ofstream file("rk4_einstein_ring.dat");

    while (theGeod.getTermCondition() == Term::Continue)
    {
        theGeod.Update();
        current_pos = theGeod.getCurrentPos();
        current_t = current_pos[0];
        current_r = current_pos[1];
        current_phi = current_pos[3];
        file << std::setprecision(15) << current_t << " " << current_r << " " << current_phi << std::endl;
    }
    file.close();
    std::cout << "Closest radius encountered: " << theGeod.getDiagnosticFinalValue()[0] << std::endl;
    double final_t = current_pos[0];
    double final_r = current_pos[1];
    double final_theta = current_pos[2];
    double final_phi = current_pos[3];
    EXPECT_NEAR(final_t, 100.792, 0.01) << "Final coordinate time differs too much from analytic value 100.792: " << final_t << std::endl;
    EXPECT_NEAR(final_phi, 4. * pi, 0.01) << "Final phi angle differs too much from 4 pi: " << final_phi << std::endl;
    EXPECT_NEAR(final_theta, pi / 2.0, 0.000000001) << "Final theta position not close enough to pi/2 after 5 half-orbits: " << final_theta << std::endl;

    double distance = sqrt((final_t - 100.792) * (final_t - 100.792) + final_r * final_r + 400. - 40. * final_r * cos(final_phi));
    std::cout << "Distance metric: " << distance << std::endl;
    EXPECT_NEAR(distance, 0., 0.1) << "Distance metric too large: " << distance << std::endl;
}

TEST(Integrators, verlet_einstein_ring)
{
    std::unique_ptr<Metric> theM = std::unique_ptr<Metric>(new KerrMetric(0., false, 1.));
    std::unique_ptr<Source> theS = std::unique_ptr<Source>(new NoSource(theM.get()));

    ClosestRadiusDiagnostic::DiagOptions =
        std::unique_ptr<ClosestRadiusOptions>(new ClosestRadiusOptions{false, UpdateFrequency{1, false, false}});

    DiagBitflag AllDiags = Diag_ClosestRadius;
    DiagBitflag ValDiag = Diag_ClosestRadius;

    TermBitflag AllTerms = Term_BoundarySphere;

    BoundarySphereTermination::TermOptions =
        std::unique_ptr<BoundarySphereTermOptions>(new BoundarySphereTermOptions{20., false, 1});

    GeodesicIntegratorFunc theIntegrator = Integrators::IntegrateGeodesicStep_Verlet;
    Integrators::IntegratorDescription = "Verlet";
    Integrators::epsilon = 0.02;

    Geodesic theGeod(theM.get(), theS.get(), AllDiags, ValDiag, AllTerms,
                     theIntegrator);

    // Set up initial conditions for the bound geodesic
    Point initpos = {0.0, 20.0, pi / 2.0, 0.0};
    double xi = 0.24904964;

    TwoIndex gdd = theM->getMetric_dd(initpos);

    OneIndex initvel = {1.0 / sqrt(-gdd[0][0]), -cos(xi) / sqrt(gdd[1][1]), 0.0, sin(xi) / sqrt(gdd[3][3])};
    ScreenIndex scrindex = {0, 0};

    theGeod.Reset(scrindex, initpos, initvel);

    Point current_pos{initpos};
    double current_t{0.0}, current_phi{0.0}, current_r{20.0};
    std::ofstream file("verlet_einstein_ring.dat");

    while (theGeod.getTermCondition() == Term::Continue)
    {
        theGeod.Update();
        current_pos = theGeod.getCurrentPos();
        current_t = current_pos[0];
        current_r = current_pos[1];
        current_phi = current_pos[3];
        file << std::setprecision(15) << current_t << " " << current_r << " " << current_phi << std::endl;
    }
    file.close();
    std::cout << "Closest radius encountered: " << theGeod.getDiagnosticFinalValue()[0] << std::endl;
    double final_t = current_pos[0];
    double final_r = current_pos[1];
    double final_theta = current_pos[2];
    double final_phi = current_pos[3];
    EXPECT_NEAR(final_t, 100.792, 2.) << "Final coordinate time differs too much from analytic value 100.792: " << final_t << std::endl;
    EXPECT_NEAR(final_phi, 4. * pi, 0.5) << "Final phi angle differs too much from 4 pi: " << final_phi << std::endl;
    EXPECT_NEAR(final_theta, pi / 2.0, 0.000000001) << "Final theta position not close enough to pi/2 after 5 half-orbits: " << final_theta << std::endl;

    double distance = sqrt((final_t - 100.792) * (final_t - 100.792) + final_r * final_r + 400. - 40. * final_r * cos(final_phi));
    std::cout << "Distance metric: " << distance << std::endl;
    EXPECT_NEAR(distance, 0., 10.) << "Distance metric too large: " << distance << std::endl;
}
