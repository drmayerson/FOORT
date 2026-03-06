///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
//////        ------ FOORT: Flexible Object Oriented Ray Tracer ------           //////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

/**
 * @file Main.cpp
 * @brief Main function for FOORT
 * @author Daniel R. Mayerson
 * @version 1.0
 * @date 2025-1-21
 * @copyright Copyright (c) 2025
 */

#include "Config.h"       // Processing configuration file (with libconfig)
#include "Diagnostics.h"  // Diagnostics
#include "Geodesic.h"     // Geodesics (and Sources)
#include "Geometry.h"     // basic tensor objects
#include "InputOutput.h"  // Output to screen and files
#include "Integrators.h"  // Integrator functions
#include "Metric.h"       // Metrics
#include "Terminations.h" // Termination conditions
#include "Utilities.h"    // Various utility functions (including timer)
#include "ViewScreen.h"   // ViewScreen (this includes Mesh objects --- ViewScreen.h includes Mesh.h)

#include <omp.h> // Needed for parallel computations with OpenMP

#include <iostream> // needed to open and load configuration file; also cout/cerr

int main(int argc, char *argv[])
{
    // This will be reset by the value specified in the configuration file,
    // but until then use the maximum level of output
    SetOutputLevel(OutputLevel::Level_4_DEBUG);

    ScreenOutput("FOORT compiled in configuration mode.", OutputLevel::Level_1_PROC);

    // We start by reading in the specified configuration file
    Config::ConfigCollection cfgObject;
    if (argc < 2) // no configuration file given
    {
        ScreenOutput("No configuration file given. Exiting...\n", OutputLevel::Level_0_WARNING);
        exit(0);
    }
    else
    {
        std::string configpath{argv[1]};
        try
        {
            // Try to open and read the configuration file
            if (!cfgObject.ReadFile(configpath.c_str()))
            {
                // File not found
                std::cerr << "Config file not found at " << configpath << ".\n Exiting...\n";
                exit(0);
            }
        }
        catch (const ConfigReader::ConfigReaderException &configex)
        {
            // Something is wrong in the configuration file (syntax error)
            std::cerr << configex.what() << "\n"
                      << "Remember that all numbers must be given as values, e.g. \"3.14/2.0\" is not allowed.\nExiting...\n";
            exit(0);
        }
    }
    // If we have made it here, the configuration file is present and its syntax is correct,
    // so we can start reading it in.
    // In general, we always use the libconfig method:
    // cfgObject.LookupValue("valuename", vartoput) -> returns false if valuename doesn't exist
    // to look up a certain configuration option. Note that if false is returned, vartoput
    // remains unaltered from the state it was in before calling LookupValue

    ScreenOutput("Initializing all object using configuration file...", OutputLevel::Level_1_PROC);

    // Initialize screen output first since it sets OutputLevel for the rest
    Config::InitializeScreenOutput(cfgObject);

    // Initialize Metric
    std::unique_ptr<Metric> theM = Config::GetMetric(cfgObject);

    // Initialize Source
    std::unique_ptr<Source> theS = Config::GetSource(cfgObject, theM.get());

    // Initialize Diagnostics (InitializeDiagnostics returns the bitflags and initializes the appropriate
    // static DiagnosticOptions structs)
    DiagBitflag AllDiags, ValDiag;
    Config::InitializeDiagnostics(cfgObject, AllDiags, ValDiag, theM.get());

    std::cout << "Made it past diagnostics initialization.\n";
    // Initialize Terminations (InitializeTerminations returns the bitflags and initializes the appropriate
    // static TerminationOptions structs)
    TermBitflag AllTerms;
    Config::InitializeTerminations(cfgObject, AllTerms, theM.get());

    // Initialize ViewScreen (this also initializes the Mesh as part of the ViewScreen)
    std::unique_ptr<ViewScreen> theView = Config::GetViewScreen(cfgObject, ValDiag, theM.get());

    // Initialize Integrator
    GeodesicIntegratorFunc theIntegrator = Config::GetGeodesicIntegrator(cfgObject);

    // Initialize Output Handler
    // First we get the info string to place at the first line of every output file
    std::string FirstLineInfo{Utilities::GetFirstLineInfoString(theM.get(), theS.get(), AllDiags, ValDiag, AllTerms, theView.get())};
    std::unique_ptr<GeodesicOutputHandler> theOutputHandler = Config::GetOutputHandler(cfgObject, AllDiags, ValDiag, FirstLineInfo);

    // Done initializing everything!
    ScreenOutput("Done loading options from configuration file.", OutputLevel::Level_1_PROC);

    // Now, we proceed to list all objects that have been initialized (using their description string)

    OutputLevel listallobjects = OutputLevel::Level_2_SUBPROC;
    ScreenOutput("\n--------------------------------", listallobjects);
    ScreenOutput("LIST OF ALL INITIALIZED OBJECTS:", listallobjects);

    ScreenOutput("Metric: " + theM->getFullDescriptionStr() + ".", listallobjects);

    ScreenOutput("Geodesic source: " + theS->getFullDescriptionStr() + ".", listallobjects);

    ScreenOutput("Diagnostics turned on: ", listallobjects);
    ScreenOutput("<begin list>", listallobjects);
    { // temp scope to create/destroy this Diagnostic vector
        DiagnosticUniqueVector tempdiagvec{CreateDiagnosticVector(AllDiags, ValDiag, nullptr)};
        for (auto &d : tempdiagvec)
        {
            ScreenOutput(d->getFullDescriptionStr() + ".", listallobjects);
        }
    }
    ScreenOutput("<end list>", listallobjects);

    ScreenOutput("Terminations turned on:", listallobjects);
    ScreenOutput("<begin list>", listallobjects);
    { // temp scope to create/destroy this Termination vector
        TerminationUniqueVector temptermvec{CreateTerminationVector(AllTerms, nullptr)};
        for (auto &t : temptermvec)
        {
            ScreenOutput(t->getFullDescriptionStr() + ".", listallobjects);
        }
        ScreenOutput("<end list>", listallobjects);
    }

    ScreenOutput(theView->getFullDescriptionStr() + ".", listallobjects);

    ScreenOutput(Integrators::GetFullIntegratorDescription(), listallobjects);

    ScreenOutput(theOutputHandler->getFullDescriptionStr(), listallobjects);

    ScreenOutput("--------------------------------\n", listallobjects);

    // totalTimer keeps track of the total time elapsed in integration
    Utilities::Timer totalTimer;
    totalTimer.reset();

    // STARTING GEODESIC INTEGRATION

    // start new iteration of integrating geodesics. ViewScreen (through Mesh) will return true when it does not
    // want to integrate another iteration of geodesics.
    while (!theView->IsFinished())
    {
        ScreenOutput("Starting new integration loop.", OutputLevel::Level_1_PROC);

        // Time this iteration of geodesics
        Utilities::Timer IterationTimer;

        // How many geodesics are we integrating this iteration
        // OpenMP distributed for loops demand a SIGNED integral type as the loop iterator
        long long CurNrGeod = static_cast<long long>(theView->getCurNrGeodesics());

        // Counter of number of geodesics already integrated in thread 0
        long long masterIndexCounter{0};

#pragma omp parallel // start up threads!
        {
#pragma omp single // only output start message and reset timer in single thread; other threads wait until OutputHandler is ready!
            {
                ScreenOutput("Integrating " + std::to_string(CurNrGeod) + " geodesics on " + std::to_string(omp_get_num_threads()) + " threads...",
                             OutputLevel::Level_1_PROC);
                IterationTimer.reset();

                // Prepare the output handler for the output to come
                theOutputHandler->PrepareForOutput(static_cast<largecounter>(CurNrGeod));
            }

            // Create one Geodesic instance per thread to work with
            Geodesic theGeod(theM.get(), theS.get(), // Metric and Source (non-owner pointers!)
                             AllDiags, ValDiag,      // Bitflags for Diagnostics
                             AllTerms,               // Bitflag for Terminations
                             theIntegrator);         // Function to use to integrate geodesic equation

            // distribute for loop iterations over threads
#pragma omp for
            for (long long index = 0; index < CurNrGeod; ++index)
            {

                // Keep count of number of geodesics integrated in thread 0 and
                // output loop progress message if applicable
                if (omp_get_thread_num() == 0)
                {
                    ++masterIndexCounter;

                    int numthreads{omp_get_num_threads()};
                    if (masterIndexCounter > 0 && masterIndexCounter % (GetLoopMessageFrequency() / numthreads) == 0)
                    {
                        double speed = masterIndexCounter * numthreads / IterationTimer.elapsed();
                        ScreenOutput("Approx. at geodesic " + std::to_string(masterIndexCounter * numthreads) + " (" + std::to_string(IterationTimer.elapsed()) + "s elapsed; speed: " + std::to_string(static_cast<long>(speed)) + " geod/s; est. loop time remaining: " + std::to_string((CurNrGeod - masterIndexCounter * numthreads) / speed) + "s)...", OutputLevel::Level_2_SUBPROC);
                    }
                }

                // Set up initial conditions for a geodesic
                Point initpos;
                OneIndex initvel;
                ScreenIndex scrindex;

                // Note that SetNewInitialConditions is a const member function, both of ViewScreen
                // and (called within) of the underlying Mesh objects; it only accesses ViewScreen/Mesh data without changing
                // anything. Therefore this does not need to be called with #pragma omp critical
                theView->SetNewInitialConditions(static_cast<largecounter>(index), initpos, initvel, scrindex);

                // Set the Geodesic to the current screen index and initial position/velocity
                theGeod.Reset(scrindex, initpos, initvel);

                // Loop integrating the geodesic step by step until finished
                while (theGeod.getTermCondition() == Term::Continue)
                {
                    theGeod.Update();
                }

                // The geodesic has finished integrating.
                // We tell the ViewScreen it is finished and give it the "values" to associate to the geodesic.
                // We pass the geodesics' output to the Output Handler
                // Note: both of these calls involve a change of internal state of ViewScreen/Mesh and OutputHandler.
                // However, they have been set up to be thread-safe, i.e. these calls will modify values in existing
                // vectors but never reshape the underlying objects!
                // Since they are thread-safe, no omp critical directive is necessary here.
                theView->GeodesicFinished(static_cast<largecounter>(index), std::move(theGeod.getDiagnosticFinalValue()));
                theOutputHandler->NewGeodesicOutput(static_cast<largecounter>(index), std::move(theGeod.getAllOutputStr()));

            } // end parallel distributed for loop over all geodesics to integrate

#pragma omp barrier // To make sure all threads are done before we output that we are done!
#pragma omp single  // only output time taken in one thread, the rest needs to wait here before exiting!
            {
                double timetaken = IterationTimer.elapsed();
                double totaltime = totalTimer.elapsed();
                ScreenOutput("Integration loop done. Time taken for integration loop: " + std::to_string(timetaken) + "s (" + std::to_string(timetaken / 60) + "m); total time elapsed: " + std::to_string(totaltime) + "s (" + std::to_string(totaltime / 60) + "m).", OutputLevel::Level_1_PROC);
            }
        } // end parallel (close threads)
        // This triggers the end of the current iteration of geodesics in ViewScreen and its Mesh;
        // the Mesh will then evaluate if it wants another iteration of geodesics to integrate and
        // set the next iteration up
        theView->EndCurrentLoop();
    } // end while

    // We are completely done integrating!
    double totaltime = totalTimer.elapsed();
    ScreenOutput("All integration finished! Total time elapsed: " + std::to_string(totaltime) + "s (" + std::to_string(totaltime / 60) + "m).", OutputLevel::Level_1_PROC);

    // Make sure to call OutputFinished() so that the OutputHandler knows to write all remaining cached geodesic info to file
    theOutputHandler->OutputFinished();

    // end main - program finished!
    ScreenOutput("FOORT finished. Goodbye!", OutputLevel::Level_1_PROC);

    return 0;
}
