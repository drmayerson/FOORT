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
TEST(Integrators, rk4_bound_geodesic)
{
    std::unique_ptr<Metric> theM = std::unique_ptr<Metric>(new KerrMetric(0., false, 1.));
    std::unique_ptr<Source> theS = std::unique_ptr<Source>(new NoSource(theM.get()));

    DiagBitflag AllDiags = Diag_FourColorScreen;
    DiagBitflag ValDiag = Diag_FourColorScreen;

    GeodesicPositionDiagnostic::DiagOptions =
        std::unique_ptr<GeodesicPositionOptions>(new GeodesicPositionOptions{5000, UpdateFrequency{1, false, false}});

    TermBitflag AllTerms = Term_BoundarySphere | Term_Horizon | Term_TimeOut;

    HorizonTermination::TermOptions =
        std::unique_ptr<HorizonTermOptions>(new HorizonTermOptions{
            dynamic_cast<SphericalHorizonMetric *>(theM.get())->getHorizonRadius(), false, 0.01, 1});
    BoundarySphereTermination::TermOptions =
        std::unique_ptr<BoundarySphereTermOptions>(new BoundarySphereTermOptions{10, false, 1});
    TimeOutTermination::TermOptions =
        std::unique_ptr<TimeOutTermOptions>(new TimeOutTermOptions{10000, 1});

    GeodesicIntegratorFunc theIntegrator = Integrators::IntegrateGeodesicStep_RK4;
    Integrators::IntegratorDescription = "RK4";
    Integrators::epsilon = 0.03;

    Geodesic theGeod(theM.get(), theS.get(), AllDiags, ValDiag, AllTerms,
                     theIntegrator);

    // Set up initial conditions for the bound geodesic
    Point initpos = {0.0, 3.0, pi / 2.0, 0.0};

    TwoIndex gdd = theM->getMetric_dd(initpos);
    OneIndex initvel = {-1.0 / sqrt(-gdd[0][0]), 0.0, 0.0, 1.0 / sqrt(gdd[3][3])};
    ScreenIndex scrindex = {0, 0};

    theGeod.Reset(scrindex, initpos, initvel);

    Point current_pos{initpos};
    double current_phi{0.0}, current_r{3.0};
    std::ofstream file("rk4_bound_geodesic.dat");
    while (theGeod.getTermCondition() == Term::Continue && current_phi < 5 * pi)
    {
        theGeod.Update();
        current_pos = theGeod.getCurrentPos();
        current_r = current_pos[1];
        current_phi = current_pos[3];
        file << std::setprecision(15) << current_r << " " << current_phi << std::endl;
    }
    file.close();
    double final_r = current_pos[1];
    double final_theta = current_pos[2];
    EXPECT_NEAR(final_r, 3.0, 0.003) << "Final radial position too large after 5 half-orbits: " << final_r << std::endl;
    EXPECT_NEAR(final_theta, pi / 2.0, 0.000000001) << "Final theta position not close enough to pi/2 after 5 half-orbits: " << final_theta << std::endl;
}

//! Test the Verlet integration of the bound circular geodesic at r=3M, which in theory should remain at r=3M.
//! The test fails if the final radial coordinate differs by 1e-3 from 3M after 5 half orbits, i.e. after the azimuthal
//! coordinate changed by 5 pi OR if the theta polar coordinate changed by more than 1e-9.
TEST(Integrators, verlet_bound_geodesic)
{
    std::unique_ptr<Metric> theM = std::unique_ptr<Metric>(new KerrMetric(0., false, 1.));
    std::unique_ptr<Source> theS = std::unique_ptr<Source>(new NoSource(theM.get()));

    DiagBitflag AllDiags = Diag_FourColorScreen;
    DiagBitflag ValDiag = Diag_FourColorScreen;

    GeodesicPositionDiagnostic::DiagOptions =
        std::unique_ptr<GeodesicPositionOptions>(new GeodesicPositionOptions{5000, UpdateFrequency{1, false, false}});

    TermBitflag AllTerms = Term_BoundarySphere | Term_Horizon | Term_TimeOut;

    HorizonTermination::TermOptions =
        std::unique_ptr<HorizonTermOptions>(new HorizonTermOptions{
            dynamic_cast<SphericalHorizonMetric *>(theM.get())->getHorizonRadius(), false, 0.01, 1});
    BoundarySphereTermination::TermOptions =
        std::unique_ptr<BoundarySphereTermOptions>(new BoundarySphereTermOptions{10, false, 1});
    TimeOutTermination::TermOptions =
        std::unique_ptr<TimeOutTermOptions>(new TimeOutTermOptions{10000, 1});

    GeodesicIntegratorFunc theIntegrator = Integrators::IntegrateGeodesicStep_Verlet;
    Integrators::IntegratorDescription = "Verlet";
    Integrators::epsilon = 0.03;

    Geodesic theGeod(theM.get(), theS.get(), AllDiags, ValDiag, AllTerms,
                     theIntegrator);

    // Set up initial conditions for the bound geodesic
    Point initpos = {0.0, 3.0, pi / 2.0, 0.0};

    TwoIndex gdd = theM->getMetric_dd(initpos);
    OneIndex initvel = {-1.0 / sqrt(-gdd[0][0]), 0.0, 0.0, 1.0 / sqrt(gdd[3][3])};
    ScreenIndex scrindex = {0, 0};

    theGeod.Reset(scrindex, initpos, initvel);

    Point current_pos{initpos};
    double current_phi{0.0}, current_r{3.0};
    std::ofstream file("verlet_bound_geodesic.dat");
    while (theGeod.getTermCondition() == Term::Continue && current_phi < 5 * pi)
    {
        theGeod.Update();
        current_pos = theGeod.getCurrentPos();
        current_r = current_pos[1];
        current_phi = current_pos[3];
        file << std::setprecision(15) << current_r << " " << current_phi << std::endl;
    }
    file.close();
    double final_r = current_pos[1];
    double final_theta = current_pos[2];
    EXPECT_NEAR(final_r, 3.0, 0.003) << "Final radial position too large after 5 half-orbits: " << final_r << std::endl;
    EXPECT_NEAR(final_theta, pi / 2.0, 0.000000001) << "Final theta position not close enough to pi/2 after 5 half-orbits: " << final_theta << std::endl;
}
