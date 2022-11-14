#include <iostream>
#include<chrono>

#include "InputOutput.h"
#include "Geometry.h"
#include"Metric.h"
#include"Integrators.h"
#include"Diagnostics.h"
#include"Config.h"

#include<random>

#include<libconfig.h++>

#include<omp.h>

class Timer
{
private:
    // Type aliases to make accessing nested type easier
    using Clock = std::chrono::steady_clock;
    using Second = std::chrono::duration<double, std::ratio<1> >;

    std::chrono::time_point<Clock> m_beg{ Clock::now() };

public:
    void reset()
    {
        m_beg = Clock::now();
    }

    double elapsed() const
    {
        return std::chrono::duration_cast<Second>(Clock::now() - m_beg).count();
    }
};

Point InputPoint()
{
    std::cout << "Enter a point: ";
    Point newpt{};
    for (auto& mem : newpt)
        std::cin >> mem;
    return newpt;
}



int main(int argc, char* argv[])
{
    SetOutputLevel(OutputLevel::Level_4_DEBUG);


    Config::ConfigObject cfgObject;
    if (argc < 2) // no configuration file given
    {
        ScreenOutput("No configuration file given. Proceeding with default configuration.\n", OutputLevel::Level_0_NONE);
    }
    else
    {
        std::string configpath{ argv[1] };
        try
        {
            cfgObject.readFile(configpath.c_str());
        }
        catch (const libconfig::FileIOException& fioex) {
            std::cerr << "Config file not found at " << configpath << ".\n Exiting...\n";
            exit(0);
        }
        catch (const libconfig::ParseException& pex) {
            std::cerr << "Unable to parse config file correctly: parse error at "
                << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError() << ".\n"
                << "Remember that all numbers must be given as values, e.g. \"3.14/2.0\" is now allowed.\nExiting...\n";
            exit(0);
        }
    }

    // cfgObject.lookupValue("valuename", vartoput) -> returns false if vartoput doesn't exist



    OutputLevel dbgoutput = OutputLevel::Level_4_DEBUG;

    // Metric
    std::unique_ptr<Metric> theM = Config::GetMetric(cfgObject);
    ScreenOutput("Metric is: " + std::string{ typeid(*theM).name() } + ".", dbgoutput);

    // Source
    std::unique_ptr<Source> theS = Config::GetSource(cfgObject,theM.get());
    ScreenOutput("Geodesic source is: " + std::string{ typeid(*theS).name() } + ".", dbgoutput);

    // Diagnostics
    DiagBitflag AllDiags, ValDiag;
    Config::InitializeDiagnostics(cfgObject, AllDiags, ValDiag);
    ScreenOutput("All Diagnostics that have been turned on are:", dbgoutput);
    ScreenOutput("<begin list>", dbgoutput);
    { // temp scope to create/destroy this diagnostic vector
        DiagnosticUniqueVector tempdiagvec{ CreateDiagnosticVector(AllDiags, ValDiag) };
        for (auto& d : tempdiagvec)
        {
            ScreenOutput("Diagnostic: " + std::string{ typeid(*d).name() } + ".", dbgoutput);
        }
        ScreenOutput("<end list>", dbgoutput);
    }

    // Terminations
    TermBitflag AllTerms;
    Config::InitializeTerminations(cfgObject, AllTerms, theM.get());
    ScreenOutput("All Terminations that have been turned on are:", dbgoutput);
    ScreenOutput("<begin list>", dbgoutput);
    { // temp scope to create/destroy this termination vector
        TerminationUniqueVector temptermvec{ CreateTerminationVector(AllTerms) };
        for (auto& t : temptermvec)
        {
            ScreenOutput("Termination: " + std::string{ typeid(*t).name() } + ".", dbgoutput);
        }
        ScreenOutput("<end list>", dbgoutput);
    }

    /*std::string tempstr;

    tempstr = (AllDiags & Diag_FourColorScreen) ? "on" : "off";
    ScreenOutput("FourColorScreen is: " + tempstr + ".", OutputLevel::Level_4_DEBUG);
    ScreenOutput("Options set: Update every N steps: " + std::to_string(FourColorScreenDiagnostic::DiagOptions->UpdateEveryNSteps),
        OutputLevel::Level_4_DEBUG);

    tempstr = (AllTerms & Term_BoundarySphere) ? "on" : "off";
    ScreenOutput("BoundarySphere is: " + tempstr + ".", OutputLevel::Level_4_DEBUG);
    ScreenOutput("Options set: Update every N steps: " + std::to_string(BoundarySphereTermination::TermOptions->UpdateEveryNSteps)
        + " and sphere radius: " + std::to_string(BoundarySphereTermination::TermOptions->SphereRadius),
        OutputLevel::Level_4_DEBUG);

    tempstr = (AllTerms & Term_TimeOut) ? "on" : "off";
    ScreenOutput("TimeOut is: " + tempstr + ".", OutputLevel::Level_4_DEBUG);
    ScreenOutput("Options set: Update every N steps: " + std::to_string(TimeOutTermination::TermOptions->UpdateEveryNSteps)
        + " and max steps: " + std::to_string(TimeOutTermination::TermOptions->MaxSteps),
        OutputLevel::Level_4_DEBUG);

    tempstr = (theM->InternalTerminate({ 0,2,0,0 }) == Term::Horizon) ? "yes" : "no";
    ScreenOutput("Terminate at r = 2? " + tempstr,
        OutputLevel::Level_4_DEBUG);

    tempstr = (theM->InternalTerminate({ 0,1.01,0,0 }) == Term::Horizon) ? "yes" : "no";
    ScreenOutput("Terminate at r = 1.01? " + tempstr,
        OutputLevel::Level_4_DEBUG);*/

    // Viewscreen
    std::unique_ptr<ViewScreen> theView = Config::GetViewScreen(cfgObject, ValDiag, theM.get());
    ScreenOutput("ViewScreen initialized. In the first iteration, it wants to integrate: "
        + std::to_string(theView->getCurNrGeodesics()) + " geodesics.", dbgoutput);

    // Integrator
    GeodesicIntegratorFunc theIntegrator = Config::GetGeodesicIntegrator(cfgObject);
    ScreenOutput("Integrator selected. Default step size: " + std::to_string(Integrators::epsilon) + ".", dbgoutput);

    // Output handler
    std::unique_ptr<GeodesicOutputHandler> theOutputHandler = Config::GetAndInitializeOutput(cfgObject);
    ScreenOutput("Geodesic output handler initialized.", dbgoutput);


    //Point thept{ InputPoint() };
    //std::cout << "Point: " << toString(thept) << "\n"
    //    << "Metric here: " << toString(theM->getMetric_dd(thept)) << "\n"
    //    << "Inverse metric here: " << toString(theM->getMetric_uu(thept)) << "\n"
    //    << "Christoffel here: " << toString(theM->getChristoffel_udd(thept)) << "\n";


    //return 0;

    Timer totalTimer;
    totalTimer.reset();
    while (!theView->IsFinished())
    {
        ScreenOutput("Starting new integration loop.", OutputLevel::Level_0_NONE);

        Timer IterationTimer;
        int CurNrGeod = theView->getCurNrGeodesics();
#pragma omp parallel
        { // start up threads!
#pragma omp master // only output start message and reset timer in master thread
            {
                ScreenOutput("Integrating " + std::to_string(CurNrGeod) + " geodesics on "
                    + std::to_string(omp_get_num_threads()) + " threads...",
                    OutputLevel::Level_0_NONE);
                IterationTimer.reset();
            }

#pragma omp for
            for (int index = 0; index < CurNrGeod; index++)
            {
                // Create geodesic
                Point initpos;
                OneIndex initvel;
                ScreenIndex scrindex;
#pragma omp critical
                theView->SetNewInitialConditions(index, initpos, initvel, scrindex);
                /*ScreenOutput("ViewScreen has initialized geodesic nr. " + std::to_string(index)
                    + " at position: " + toString(initpos)
                    + " with initial velocity: " + toString(initvel) + ".", dbgoutput);*/
                    // ScreenOutput("ViewScreen has initialized geodesic with initial velocity: " + toString(initvel) + ".", dbgoutput);
                Geodesic TestGeod(scrindex, initpos, initvel, theM.get(), theS.get(), AllDiags, ValDiag, AllTerms, theIntegrator);

                // Loop geodesic until finished
                while (TestGeod.GetTermCondition() == Term::Continue)
                {
                    TestGeod.Update();
                }
                // ScreenOutput("Final geodesic is at position: " + toString(TestGeod.getCurrentPos()) + ".", dbgoutput);
                /*ScreenOutput("After " + std::to_string(i) + " steps, geodesic is now at position : " + toString(TestGeod.getCurrentPos())
                    + " with velocity: " + toString(TestGeod.getCurrentVel())
                    + " and Termination condition: " + std::to_string(static_cast<int>(TestGeod.GetTermCondition()))
                    + ".", dbgoutput);
                ScreenOutput("Geodesic output string is:", dbgoutput);
                ScreenOutput(TestGeod.getDiagnosticOutputStr(),dbgoutput);*/
#pragma omp critical
                {
                    theView->GeodesicFinished(index, TestGeod.GetDiagnosticFinalValue());
                    theOutputHandler->NewGeodesicOutput(TestGeod.getDiagnosticOutputStr());
                }

            } // end for

#pragma omp single // only output time taken in one thread, the rest needs to wait here before exiting!
            {
                // TODO: uncomment
                double timetaken = IterationTimer.elapsed();
                double totaltime = totalTimer.elapsed();
                ScreenOutput("Integration loop done. Time taken for integration loop: "
                    + std::to_string(timetaken) + "s (" + std::to_string(timetaken / 60) + "m); total time elapsed: "
                    + std::to_string(totaltime) + "s (" + std::to_string(totaltime / 60) + "m).", OutputLevel::Level_0_NONE);
            }
        } // end parallel
        theView->EndCurrentLoop();
    } // end while

    double totaltime = totalTimer.elapsed();
    ScreenOutput("All integration finished! Total time elapsed: "
        + std::to_string(totaltime) + "s (" + std::to_string(totaltime / 60) + "m).", OutputLevel::Level_0_NONE);

    theOutputHandler->OutputFinished();
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu