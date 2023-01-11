///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
////// ------ FOORT: Flexible Object Oriented Ray Tracer ------                  //////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////
////// To CHANGE THE SETTNGS used in PRECOMPILED MODE (i.e. when CONFIGURATION_MODE
////// is turned off), change the parameters etc. in LoadPrecompiledOptions() below!
///////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////
////// MAIN.CPP
////// This file contains the main() function,
////// and also the function LoadPrecompiledOptions(), which is used when
////// compiling in pre-compiled mode.
///////////////////////////////////////////////////////////////////////////////////////


#include "Geometry.h"           // basic tensor objects
#include "Metric.h"             // Metrics
#include "Diagnostics.h"        // Diagnostics
#include "Terminations.h"       // Termination conditions
#include "Geodesic.h"           // Geodesics (and Sources)
#include "ViewScreen.h"         // ViewScreen (this includes Mesh objects --- ViewScreen.h includes Mesh.h)
#include "Integrators.h"        // Integrator functions
#include "InputOutput.h"        // Output to screen and files
#include "Utilities.h"          // Various utility functions (including timer)

#include <omp.h>                // Needed for parallel computations with OpenMP

#include <iostream>             // needed to open and load configuration file, if working in CONFIGURATION_MODE; also cout/cerr

//// CONFIGURATION_MODE is set in Config.h!
#include "Config.h"             // Processing configuration file (with libconfig)

#ifdef CONFIGURATION_MODE       // This is set in Config.h!
#include <libconfig.h++>         // Needed to load configuration file, if working in CONFIGURATION_MODE
#endif




///////////////////////////////////////////////////////////////////////////////////////
////// PRECOMPILED SETTINGS SPECIFIED IN THIS FUNCTION                           //////
///////////////////////////////////////////////////////////////////////////////////////

// This function is called if CONFIGURATION_MODE is NOT turned on.
void LoadPrecompiledOptions(std::unique_ptr<Metric> &theM, std::unique_ptr<Source> &theS, DiagBitflag &AllDiags, DiagBitflag &ValDiag,
    TermBitflag &AllTerms, std::unique_ptr<ViewScreen> &theView, GeodesicIntegratorFunc &theIntegrator,
    std::unique_ptr<GeodesicOutputHandler> & theOutputHandler)
{
    //// Screen output level ////
    SetOutputLevel(OutputLevel::Level_4_DEBUG);
    // Frequency of messages during each integration loop
    SetLoopMessageFrequency(LARGECOUNTER_MAX);


    ///// Metric ////
    // Syntax: KerrMetric (real a, bool rLogScale)
    // Syntax: RasheedLarsenMetric (real m, real a, real p, real q, bool rLogScale)
    // Syntax: FlatSpaceMetric()
    theM = std::unique_ptr<Metric>(new KerrMetric( 0.5, false ));

    //// Source ////
    // Syntax: NoSource(const Metric *)
    theS = std::unique_ptr<Source>(new NoSource(theM.get()));


    //// Diagnostics ////
    // Flag possibilities: Diag_FourColorScreen, Diag_GeodesicPosition, Diag_EquatorialPasses
    AllDiags = Diag_FourColorScreen | Diag_EquatorialPasses;
    ValDiag = Diag_EquatorialPasses;


    //// Diagnostic options (static member structs) ////
    // Syntax: UpdateFrequency{ largecounter updateEveryNSteps, bool UpdateOnStart, bool UpdateOnEnd }
    // Syntax: GeodesicPositionOptions(largecounter outputsteps, UpdateFrequency)
    // Syntax: EquatorialPassesOptions(real thethreshold, UpdateFrequency)
    // Syntax DiagnosticOptions(UpdateFrequency)
    // Note: FourColorScreen does not have any options
    GeodesicPositionDiagnostic::DiagOptions =
        std::unique_ptr<GeodesicPositionOptions>(new GeodesicPositionOptions{ 5000, UpdateFrequency{1,false,false} });
    EquatorialPassesDiagnostic::DiagOptions =
        std::unique_ptr<EquatorialPassesOptions>(new EquatorialPassesOptions{0.01, UpdateFrequency{1,false,false} });


    //// Terminations ////
    // Flag possibilities: Term_BoundarySphere, Term_Horizon, Term_TimeOut
    AllTerms = Term_BoundarySphere | Term_Horizon | Term_TimeOut;


    //// Termination options (static member structs) ////
    // Syntax: HorizonTermOptions(real HorizonRadius,bool rLogScale, real EpsAtHorizon, largecounter UpdateNSteps)
    // Syntax: BoundarySphereTermOptions(real sphereradius, largecounter UpdateNSteps)
    // Syntax TimeOutTermOptions(largecounter timeoutsteps, largecounter UpdateNSteps)
    if (dynamic_cast<SphericalHorizonMetric*>(theM.get())) // Only set the Horizon termination options if the metric has a horizon
    {
        HorizonTermination::TermOptions =
            std::unique_ptr<HorizonTermOptions>(new HorizonTermOptions{
            dynamic_cast<SphericalHorizonMetric*>(theM.get())->getHorizonRadius(), false, 0.01, 1 });
    }
    BoundarySphereTermination::TermOptions =
        std::unique_ptr<BoundarySphereTermOptions>(new BoundarySphereTermOptions{ 1000, false, 1 });
    TimeOutTermination::TermOptions =
        std::unique_ptr<TimeOutTermOptions>(new TimeOutTermOptions{ 1000000, 1 });


    //// Mesh & Viewscreen ////
    // Mesh possibilities & syntax:
    // SimpleSquareMesh(largecounter totalPixels, ValDiag);
    // InputCertainPixelsMesh(largecounter totalPixels, ValDiag); // totalPixels fixes the screen size in pixels, pixels to be integrated will be user-inputted
    // SquareSubdivisionMesh: see below
    std::unique_ptr<Mesh> theMesh = std::unique_ptr<Mesh>(new SquareSubdivisionMesh(
        0, // maxpixels (0 = infinite)
        10000, // initial pixels
        7, // maxsubdivide
        2000, // iteration pixels
        false, // initial subdivide to final
        ValDiag )); // value diagnostic to be used for calculating distances

    // ViewScreen syntax: see below
    theView = std::unique_ptr<ViewScreen>(new ViewScreen(
        { 0.0, 1000.0, 0.2966972222222, 0.0 }, // position
        { 0.0, -1.0, 0.0, 0.0 }, // direction
        { 15, 15 }, // screen size
        { 0, 0 }, // screen center
        std::move(theMesh), // R-value of Mesh --- ViewScreen becomes owner!
        theM.get())); // (non-owner) pointer to Metric


    //// Integrator ////
    theIntegrator = Integrators::IntegrateGeodesicStep_RK4; // IntegrateGeodesicStep_RK4 or IntegrateGeodesicStep_Verlet
    Integrators::IntegratorDescription = "RK4";
    Integrators::epsilon = 0.03; // base step size that is used (is adapted dynamically)


    //// Output handler ////
    // Syntax: see below
    theOutputHandler = std::unique_ptr<GeodesicOutputHandler>(new GeodesicOutputHandler(
        "output", // file prefix
        Utilities::GetTimeStampString(), // time stamp
        "dat", // file extension
        Utilities::GetDiagNameStrings(AllDiags, ValDiag), // strings of names of all Diagnostics turned on
        200000, // nr geodesics to cache
        200000, // nr geodesics per file
        Utilities::GetFirstLineInfoString(theM.get(), theS.get(), AllDiags, ValDiag, AllTerms, theView.get()) // first line info
    ));
}



///////////////////////////////////////////////////////////////////////////////////////
////// MAIN                                                                      //////
///////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // This will be reset by the value specified in the configuration file,
    // but until then use the maximum level of output
    SetOutputLevel(OutputLevel::Level_4_DEBUG);


#ifdef CONFIGURATION_MODE   // CONFIGURATION MODE (using libconfig)
    ScreenOutput("FOORT compiled in configuration mode.", OutputLevel::Level_1_PROC);

    // We are in CONFIGURATION MODE, so we start by reading in the specified configuration file
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
            // Try to open and read the configuration file
            cfgObject.readFile(configpath.c_str());
        }
        catch (const libconfig::FileIOException&)
        {
            // File not found
            std::cerr << "Config file not found at " << configpath << ".\n Exiting...\n";
            exit(0);
        }
        catch (const libconfig::ParseException& pex)
        {
            // Something is wrong in the configuration file (syntax error)
            std::cerr << "Unable to parse config file correctly: parse error at "
                << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError() << ".\n"
                << "Remember that all numbers must be given as values, e.g. \"3.14/2.0\" is now allowed.\nExiting...\n";
            exit(0);
        }
    }
    // If we have made it here, the configuration file is present and its syntax is correct,
    // so we can start reading it in.
    // In general, we always use the libconfig method:
    // cfgObject.lookupValue("valuename", vartoput) -> returns false if valuename doesn't exist
    // to look up a certain configuration option. Note that if false is returned, vartoput
    // remains unaltered from the state it was in before calling lookupValue

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
    Config::InitializeDiagnostics(cfgObject, AllDiags, ValDiag);

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
    std::string FirstLineInfo{ Utilities::GetFirstLineInfoString(theM.get(), theS.get(), AllDiags, ValDiag, AllTerms, theView.get()) };
    std::unique_ptr<GeodesicOutputHandler> theOutputHandler = Config::GetOutputHandler(cfgObject, AllDiags, ValDiag, FirstLineInfo);

    // Done initializing everything!
    ScreenOutput("Done loading options from configuration file.", OutputLevel::Level_1_PROC);

#else               // IN PRECOMPILED OPTIONS MODE
    ScreenOutput("FOORT compiled in precompiled options mode.", OutputLevel::Level_1_PROC);

    ScreenOutput("Initializing all object using precompiled configurations...", OutputLevel::Level_1_PROC);


    // Metric, Source, Diagnostics (incl. static DiagnosticOptions), Terminations (incl static TerminationOptions),
    // ViewScreen (incl. Mesh), Integrator function, Output Handler: these are all initialized now by the precompiled
    // options specified above in Main.cpp under LoadPrecompiledOptions()
    std::unique_ptr<Metric> theM;
    std::unique_ptr<Source> theS;
    DiagBitflag AllDiags, ValDiag;
    TermBitflag AllTerms;
    std::unique_ptr<ViewScreen> theView;
    GeodesicIntegratorFunc theIntegrator;
    std::unique_ptr<GeodesicOutputHandler> theOutputHandler;
    LoadPrecompiledOptions(theM, theS, AllDiags, ValDiag, AllTerms, theView, theIntegrator, theOutputHandler);

    // Done initializing everything!
    ScreenOutput("Done loading precompiled options.", OutputLevel::Level_1_PROC);

#endif              // CONFIGURATION OR PRECOMPILED MODE

    // Now, we proceed to list all objects that have been initialized (using their description string)

    OutputLevel listallobjects = OutputLevel::Level_2_SUBPROC;
    ScreenOutput("\n--------------------------------", listallobjects);
    ScreenOutput("LIST OF ALL INITIALIZED OBJECTS:", listallobjects);

    ScreenOutput("Metric: " + theM->getFullDescriptionStr() + ".", listallobjects);

    ScreenOutput("Geodesic source: " + theS->getFullDescriptionStr() + ".", listallobjects);

    ScreenOutput("Diagnostics turned on: ", listallobjects);
    ScreenOutput("<begin list>", listallobjects);
    { // temp scope to create/destroy this Diagnostic vector
        DiagnosticUniqueVector tempdiagvec{ CreateDiagnosticVector(AllDiags, ValDiag, nullptr) };
        for (auto& d : tempdiagvec)
        {
            ScreenOutput(d->getFullDescriptionStr() + ".", listallobjects);
        }
    }
    ScreenOutput("<end list>", listallobjects);

    ScreenOutput("Terminations turned on:", listallobjects);
    ScreenOutput("<begin list>", listallobjects);
    { // temp scope to create/destroy this Termination vector
        TerminationUniqueVector temptermvec{ CreateTerminationVector(AllTerms, nullptr) };
        for (auto& t : temptermvec)
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
        long long masterIndexCounter{ 0 };

#pragma omp parallel // start up threads!
        { 
#pragma omp single // only output start message and reset timer in single thread; other threads wait until OutputHandler is ready!
            {
                ScreenOutput("Integrating " + std::to_string(CurNrGeod) + " geodesics on "
                    + std::to_string(omp_get_num_threads()) + " threads...",
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

                    int numthreads{ omp_get_num_threads() };
                    if (masterIndexCounter > 0 && masterIndexCounter % (GetLoopMessageFrequency() / numthreads) == 0)
                    {
                        double speed = masterIndexCounter * numthreads / IterationTimer.elapsed();
                        ScreenOutput("Approx. at geodesic "
                            + std::to_string(masterIndexCounter * numthreads)
                            + " ("
                            + std::to_string(IterationTimer.elapsed()) + "s elapsed; speed: "
                            + std::to_string(static_cast<long>(speed)) + " geod/s; est. loop time remaining: "
                            + std::to_string((CurNrGeod - masterIndexCounter * numthreads) / speed)
                            + "s)..."
                            , OutputLevel::Level_2_SUBPROC);
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
                ScreenOutput("Integration loop done. Time taken for integration loop: "
                    + std::to_string(timetaken) + "s (" + std::to_string(timetaken / 60) + "m); total time elapsed: "
                    + std::to_string(totaltime) + "s (" + std::to_string(totaltime / 60) + "m).", OutputLevel::Level_1_PROC);
            }
        } // end parallel (close threads)
        // This triggers the end of the current iteration of geodesics in ViewScreen and its Mesh;
        // the Mesh will then evaluate if it wants another iteration of geodesics to integrate and
        // set the next iteration up
        theView->EndCurrentLoop();
    } // end while
    
    // We are completely done integrating!
    double totaltime = totalTimer.elapsed();
    ScreenOutput("All integration finished! Total time elapsed: "
        + std::to_string(totaltime) + "s (" + std::to_string(totaltime / 60) + "m).", OutputLevel::Level_1_PROC);

    // Make sure to call OutputFinished() so that the OutputHandler knows to write all remaining cached geodesic info to file
    theOutputHandler->OutputFinished();

    // end main - program finished!
    ScreenOutput("FOORT finished. Goodbye!", OutputLevel::Level_1_PROC);

    return 0; 
} 