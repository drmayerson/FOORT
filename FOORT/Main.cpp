#include <iostream>
#include<random>

#include<omp.h>

#include "InputOutput.h"
#include "Geometry.h"
#include"Metric.h"
#include"Integrators.h"
#include"Diagnostics.h"
#include"Utilities.h"
#include "Config.h"

#ifdef CONFIGURATION_MODE // This is set in Config.h!
#include<libconfig.h++>
#endif


// This function is called if CONFIGURATION_MODE is NOT turned on.
// CHANGE PRECOMPILED OPTIONS HERE!
void LoadPrecompiledOptions(std::unique_ptr<Metric> &theM, std::unique_ptr<Source> &theS, DiagBitflag &AllDiags, DiagBitflag &ValDiag,
    TermBitflag &AllTerms, std::unique_ptr<ViewScreen> &theView, GeodesicIntegratorFunc &theIntegrator,
    std::unique_ptr<GeodesicOutputHandler> & theOutputHandler)
{
    // Screen output
    SetOutputLevel(OutputLevel::Level_4_DEBUG);

    // Metric
    // KerrMetric (real a, bool rLogScale)
    theM = std::unique_ptr<Metric>(new KerrMetric( 0.5, false ));

    // Source
    theS = std::unique_ptr<Source>(new NoSource(theM.get()));

    // Diagnostics
    AllDiags = Diag_FourColorScreen | Diag_EquatorialPasses;
    ValDiag = Diag_EquatorialPasses;

    // Diagnostic options
    FourColorScreenDiagnostic::DiagOptions =
        std::unique_ptr<DiagnosticOptions>(new DiagnosticOptions{ Update_OnlyFinish });
    GeodesicPositionDiagnostic::DiagOptions =
        std::unique_ptr<GeodesicPositionOptions>(new GeodesicPositionOptions{ 5000, 1 });
    EquatorialPassesDiagnostic::DiagOptions =
        std::unique_ptr<DiagnosticOptions>(new DiagnosticOptions{ 1 });

    // Terminations
    AllTerms = Term_BoundarySphere | Term_Horizon | Term_TimeOut;

    // Termination options
    if (dynamic_cast<SphericalHorizonMetric*>(theM.get()))
    {
        HorizonTermination::TermOptions =
            std::unique_ptr<HorizonTermOptions>(new HorizonTermOptions{
            dynamic_cast<SphericalHorizonMetric*>(theM.get())->getHorizonRadius(), false, 0.01, 1 });
    }
    BoundarySphereTermination::TermOptions =
        std::unique_ptr<BoundarySphereTermOptions>(new BoundarySphereTermOptions{ 1000, 1 });
    TimeOutTermination::TermOptions =
        std::unique_ptr<TimeOutTermOptions>(new TimeOutTermOptions{ 1000000, 1 });

    // Mesh & Viewscreen
    std::unique_ptr<Mesh> theMesh = std::unique_ptr<Mesh>(new SquareSubdivisionMesh(
        -1, // maxpixels
        10000, // initial pixels
        7, // maxsubdivide
        2000, // iteration pixels
        false, // initial subdivide to final
        ValDiag ));
    theView = std::unique_ptr<ViewScreen>(new ViewScreen(
        { 0.0,1000.0,0.2966972222222, 0.0 }, // position
        { 0.0, -1.0, 0.0, 0.0 }, // direction
        { 15, 15 }, // screen size
        std::move(theMesh),
        theM.get()));

    // Integrator
    theIntegrator = Integrators::IntegrateGeodesicStep_RK4;
    Integrators::epsilon = 0.03;

    // Output handler
    theOutputHandler = std::unique_ptr<GeodesicOutputHandler>(new GeodesicOutputHandler(
        "output", // file prefix
        Utilities::GetTimeStampString(), // time stamp
        "dat", // file extension
        Utilities::GetDiagStrings(AllDiags, ValDiag),
        200000, // nr geodesics to cache
        200000, // nr geodesics per file
        Utilities::GetFirstLineInfoString(theM.get(), theS.get(), AllDiags, ValDiag, AllTerms, theView.get(),
            theIntegrator) // first line info
    ));
}



int main(int argc, char* argv[])
{
    SetOutputLevel(OutputLevel::Level_4_DEBUG);

    // CONFIGURATION MODE (using libconfig)
#ifdef CONFIGURATION_MODE
    ScreenOutput("FOORT compiled in configuration mode.", OutputLevel::Level_1_PROC);

    Config::ConfigObject cfgObject;
    if (argc < 2) // no configuration file given
    {
        ScreenOutput("No configuration file given. Exiting...\n", OutputLevel::Level_0_WARNING);
        exit(0);
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

    // Initialize screen output first since it sets OutputLevel for the rest
    Config::InitializeScreenOutput(cfgObject);

    // Metric
    std::unique_ptr<Metric> theM = Config::GetMetric(cfgObject);

    // Source
    std::unique_ptr<Source> theS = Config::GetSource(cfgObject, theM.get());

    // Diagnostics
    DiagBitflag AllDiags, ValDiag;
    Config::InitializeDiagnostics(cfgObject, AllDiags, ValDiag);

    // Terminations
    TermBitflag AllTerms;
    Config::InitializeTerminations(cfgObject, AllTerms, theM.get());

    // Viewscreen
    std::unique_ptr<ViewScreen> theView = Config::GetViewScreen(cfgObject, ValDiag, theM.get());

    // Integrator
    GeodesicIntegratorFunc theIntegrator = Config::GetGeodesicIntegrator(cfgObject);

    // Output handler
    std::string FirstLineInfo{ Utilities::GetFirstLineInfoString(theM.get(), theS.get(), AllDiags, ValDiag, AllTerms, theView.get(),
        theIntegrator) };
    std::unique_ptr<GeodesicOutputHandler> theOutputHandler = Config::InitializeOutputHandler(cfgObject, AllDiags, ValDiag, FirstLineInfo);

    ScreenOutput("Done loading options from configuration file.", OutputLevel::Level_1_PROC);

#else // IN PRECOMPILED OPTIONS MODE
    ScreenOutput("FOORT compiled in precompiled options mode.", OutputLevel::Level_1_PROC);
    ScreenOutput("Initializing all object using configuration file...", OutputLevel::Level_1_PROC);


    std::unique_ptr<Metric> theM;
    std::unique_ptr<Source> theS;
    DiagBitflag AllDiags, ValDiag;
    TermBitflag AllTerms;
    std::unique_ptr<ViewScreen> theView;
    GeodesicIntegratorFunc theIntegrator;
    std::unique_ptr<GeodesicOutputHandler> theOutputHandler;
    LoadPrecompiledOptions(theM, theS, AllDiags, ValDiag, AllTerms, theView, theIntegrator, theOutputHandler);

    ScreenOutput("Done loading precompiled options.", OutputLevel::Level_1_PROC);

#endif // CONFIGURATION OR PRECOMPILED MODE

    OutputLevel listallobjects = OutputLevel::Level_2_SUBPROC;

    ScreenOutput("Metric: " + theM->GetDescriptionString() + ".", listallobjects);
    ScreenOutput("Geodesic source: " + theS->GetDescriptionString() + ".", listallobjects);
    ScreenOutput("Diagnostics turned on: ", listallobjects);
    ScreenOutput("<begin list>", listallobjects);
    { // temp scope to create/destroy this diagnostic vector
        DiagnosticUniqueVector tempdiagvec{ CreateDiagnosticVector(AllDiags, ValDiag, nullptr) };
        for (auto& d : tempdiagvec)
        {
            ScreenOutput(d->GetDescriptionString() + ".", listallobjects);
        }
    }
    ScreenOutput("<end list>", listallobjects);
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
    ScreenOutput(theView->GetDescriptionstring() + ".", listallobjects);
    ScreenOutput("Integrator selected. Default step size: " + std::to_string(Integrators::epsilon) + ".", listallobjects);
    ScreenOutput("Geodesic output handler initialized.", listallobjects);

    Utilities::Timer totalTimer;
    totalTimer.reset();
    while (!theView->IsFinished())
    {
        ScreenOutput("Starting new integration loop.", OutputLevel::Level_1_PROC);

        Utilities::Timer IterationTimer;
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

            /////////
            // METHOD NO INTERNAL THREAD CACHING
            ////////
            

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

            
            /////////
            // END METHOD NO INTERNAL THREAD CACHING
            ////////

            /////////
            // METHOD WITH INTERNAL THREAD CACHING
            ////////
            /*

            // Cache all geodesic initial conditions first
            int mythread = omp_get_thread_num();
            int totalthreads = omp_get_num_threads();
            ThreadIntermediateCacher myCacher{ CurNrGeod / totalthreads  };
            for (int i = 0; i < CurNrGeod; ++i)
            {
                if (i % totalthreads == mythread)
                {
                    int tempindex{};
                    Point temppos{};
                    OneIndex tempvel{};
                    ScreenIndex tempscrind{};
#pragma omp critical
                    theView->SetNewInitialConditions(tempindex, temppos, tempvel, tempscrind);

                    myCacher.CacheInitialConditions(tempindex, temppos, tempvel, tempscrind);
                }
            }

            // Now loop through integrating all geodesics (no pragma omp critical needed!)
            int nrThreadGeod{ myCacher.GetNrInitialConds() };
            for (int i = 0; i < nrThreadGeod; ++i)
            {
                int tempindex{};
                Point temppos{};
                OneIndex tempvel{};
                ScreenIndex tempscrind{};
                myCacher.SetNewInitialConditions(tempindex, temppos, tempvel, tempscrind);
                Geodesic TheGeod(tempscrind, temppos, tempvel, theM.get(), theS.get(), AllDiags, ValDiag, AllTerms, theIntegrator);

                // Loop geodesic until finished
                while (TheGeod.GetTermCondition() == Term::Continue)
                {
                    TheGeod.Update();
                }

                myCacher.CacheGeodesicOutput(tempindex, TheGeod.GetDiagnosticFinalValue(), TheGeod.getAllOutputStr());
            }

            // All geodesics have been integrated. De-cache all geodesic outputs
            if (nrThreadGeod != myCacher.GetNrGeodesicOutputs())
                ScreenOutput("Did not get equal number of outputs!", OutputLevel::Level_0_WARNING);
            for (int i = 0; i < nrThreadGeod; ++i)
            {
                int tempindex{};
                std::vector<real> tempvals{};
                std::vector<std::string> tempoutput{};
                myCacher.SetGeodesicOutput(tempindex, tempvals, tempoutput);
#pragma omp critical
                {
                    theView->GeodesicFinished(tempindex, tempvals);
                    theOutputHandler->NewGeodesicOutput(tempoutput);
                }
            }
            */
            /////////
            // END METHOD WITH INTERNAL THREAD CACHING
            ////////

            // To make sure all threads are done before we output that we are done!
#pragma omp barrier
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