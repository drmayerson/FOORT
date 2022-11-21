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



int main(int argc, char* argv[])
{
    SetOutputLevel(OutputLevel::Level_4_DEBUG);

    Config::ConfigObject cfgObject;
    if (argc < 2) // no configuration file given
    {
        ScreenOutput("No configuration file given. Proceeding with default configuration.\n", OutputLevel::Level_0_WARNING);
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

    ScreenOutput("Initializing all object using configuration file...", OutputLevel::Level_1_PROC);

    OutputLevel listallobjects = OutputLevel::Level_2_SUBPROC;

    // Initialize screen output first since it sets OutputLevel for the rest
    Config::InitializeScreenOutput(cfgObject);

    // Metric
    std::unique_ptr<Metric> theM = Config::GetMetric(cfgObject);
    ScreenOutput("Metric: " + theM->GetDescriptionString() + ".", listallobjects);

    // Source
    std::unique_ptr<Source> theS = Config::GetSource(cfgObject,theM.get());
    ScreenOutput("Geodesic source: " + theS->GetDescriptionString() + ".", listallobjects);

    // Diagnostics
    DiagBitflag AllDiags, ValDiag;
    Config::InitializeDiagnostics(cfgObject, AllDiags, ValDiag);
    ScreenOutput("Diagnostics turned on: ", listallobjects);
    ScreenOutput("<begin list>", listallobjects);
    { // temp scope to create/destroy this diagnostic vector
        DiagnosticUniqueVector tempdiagvec{ CreateDiagnosticVector(AllDiags, ValDiag, nullptr) };
        for (auto& d : tempdiagvec)
        {
            ScreenOutput(d->GetDescriptionString() + ".", listallobjects);
        }
        ScreenOutput("<end list>", listallobjects);
    }

    // Terminations
    TermBitflag AllTerms;
    Config::InitializeTerminations(cfgObject, AllTerms, theM.get());
    ScreenOutput("Terminations turned on:", listallobjects);
    ScreenOutput("<begin list>", listallobjects);
    { // temp scope to create/destroy this termination vector
        TerminationUniqueVector temptermvec{ CreateTerminationVector(AllTerms, nullptr) };
        for (auto& t : temptermvec)
        {
            ScreenOutput(t->GetDescriptionString() + ".", listallobjects);
        }
        ScreenOutput("<end list>", listallobjects);
    }

    // Viewscreen
    std::unique_ptr<ViewScreen> theView = Config::GetViewScreen(cfgObject, ValDiag, theM.get());
    ScreenOutput(theView->GetDescriptionstring() + ".", listallobjects);

    // Integrator
    GeodesicIntegratorFunc theIntegrator = Config::GetGeodesicIntegrator(cfgObject);
    ScreenOutput("Integrator selected. Default step size: " + std::to_string(Integrators::epsilon) + ".", listallobjects);

    // Output handler
    std::string FirstLineInfo{ Config::GetFirstLineInfoString(theM.get(), theS.get(), AllDiags, ValDiag, AllTerms, theView.get(),
        theIntegrator) };
    std::unique_ptr<GeodesicOutputHandler> theOutputHandler = Config::InitializeOutputHandler(cfgObject,AllDiags,ValDiag,FirstLineInfo);
    ScreenOutput("Geodesic output handler initialized.", listallobjects);

    ScreenOutput("Done initialization.", OutputLevel::Level_1_PROC);



    Timer totalTimer;
    totalTimer.reset();
    while (!theView->IsFinished())
    {
        ScreenOutput("Starting new integration loop.", OutputLevel::Level_1_PROC);

        Timer IterationTimer;
        int CurNrGeod = theView->getCurNrGeodesics();
#pragma omp parallel
        { // start up threads!
#pragma omp master // only output start message and reset timer in master thread
            {
                ScreenOutput("Integrating " + std::to_string(CurNrGeod) + " geodesics on "
                    + std::to_string(omp_get_num_threads()) + " threads...",
                    OutputLevel::Level_1_PROC);
                IterationTimer.reset();
            }

#pragma omp for
            for (int index = 0; index < CurNrGeod; ++index)
            {
                // Create geodesic
                Point initpos;
                OneIndex initvel;
                ScreenIndex scrindex;
#pragma omp critical
                theView->SetNewInitialConditions(index, initpos, initvel, scrindex);
                Geodesic TestGeod(scrindex, initpos, initvel, theM.get(), theS.get(), AllDiags, ValDiag, AllTerms, theIntegrator);

                // Loop geodesic until finished
                while (TestGeod.GetTermCondition() == Term::Continue)
                {
                    TestGeod.Update();
                }
#pragma omp critical
                {
                    theView->GeodesicFinished(index, TestGeod.GetDiagnosticFinalValue());
                    theOutputHandler->NewGeodesicOutput(TestGeod.getAllOutputStr());
                }

            } // end for

#pragma omp single // only output time taken in one thread, the rest needs to wait here before exiting!
            {
                double timetaken = IterationTimer.elapsed();
                double totaltime = totalTimer.elapsed();
                ScreenOutput("Integration loop done. Time taken for integration loop: "
                    + std::to_string(timetaken) + "s (" + std::to_string(timetaken / 60) + "m); total time elapsed: "
                    + std::to_string(totaltime) + "s (" + std::to_string(totaltime / 60) + "m).", OutputLevel::Level_1_PROC);
            }
        } // end parallel
        theView->EndCurrentLoop();
    } // end while

    double totaltime = totalTimer.elapsed();
    ScreenOutput("All integration finished! Total time elapsed: "
        + std::to_string(totaltime) + "s (" + std::to_string(totaltime / 60) + "m).", OutputLevel::Level_1_PROC);

    theOutputHandler->OutputFinished();
}