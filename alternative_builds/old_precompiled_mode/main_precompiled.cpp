// This part used to be in main.cpp, in case the libconfig library (no longer used) was not available.
// Additionally, the majority of Config.cpp (except for the std::uniquepointers at the top) used to be in a  #ifdef CONFIGURATION_MODE ... #endif block.
// CONFIGURATION_MODE itself was #defined in Config.h.
// The below was in main.cpp

///////////////////////////////////////////////////////////////////////////////////////
////// To CHANGE THE SETTNGS used in PRECOMPILED MODE (i.e. when CONFIGURATION_MODE
////// is turned off), change the parameters etc. in LoadPrecompiledOptions() below!
///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////
////// PRECOMPILED SETTINGS SPECIFIED IN THIS FUNCTION                           //////
///////////////////////////////////////////////////////////////////////////////////////

// This function is called if CONFIGURATION_MODE is NOT turned on.
void LoadPrecompiledOptions(std::unique_ptr<Metric> &theM, std::unique_ptr<Source> &theS, DiagBitflag &AllDiags, DiagBitflag &ValDiag,
                            TermBitflag &AllTerms, std::unique_ptr<ViewScreen> &theView, GeodesicIntegratorFunc &theIntegrator,
                            std::unique_ptr<GeodesicOutputHandler> &theOutputHandler)
{
    //// Screen output level ////
    SetOutputLevel(OutputLevel::Level_4_DEBUG);
    // Frequency of messages during each integration loop
    SetLoopMessageFrequency(LARGECOUNTER_MAX);

    ///// Metric ////
    // Syntax: KerrMetric (real a, bool rLogScale)
    // Syntax: RasheedLarsenMetric (real m, real a, real p, real q, bool rLogScale)
    // Syntax: FlatSpaceMetric()
    theM = std::unique_ptr<Metric>(new KerrMetric(0.5, false, 1.));

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
        std::unique_ptr<GeodesicPositionOptions>(new GeodesicPositionOptions{5000, UpdateFrequency{1, false, false}});
    EquatorialPassesDiagnostic::DiagOptions =
        std::unique_ptr<EquatorialPassesOptions>(new EquatorialPassesOptions{0.01, UpdateFrequency{1, false, false}});

    //// Terminations ////
    // Flag possibilities: Term_BoundarySphere, Term_Horizon, Term_TimeOut
    AllTerms = Term_BoundarySphere | Term_Horizon | Term_TimeOut;

    //// Termination options (static member structs) ////
    // Syntax: HorizonTermOptions(real HorizonRadius,bool rLogScale, real EpsAtHorizon, largecounter UpdateNSteps)
    // Syntax: BoundarySphereTermOptions(real sphereradius, largecounter UpdateNSteps)
    // Syntax TimeOutTermOptions(largecounter timeoutsteps, largecounter UpdateNSteps)
    if (dynamic_cast<SphericalHorizonMetric *>(theM.get())) // Only set the Horizon termination options if the metric has a horizon
    {
        HorizonTermination::TermOptions =
            std::unique_ptr<HorizonTermOptions>(new HorizonTermOptions{
                dynamic_cast<SphericalHorizonMetric *>(theM.get())->getHorizonRadius(), false, 0.01, 1});
    }
    BoundarySphereTermination::TermOptions =
        std::unique_ptr<BoundarySphereTermOptions>(new BoundarySphereTermOptions{1000, false, 1});
    TimeOutTermination::TermOptions =
        std::unique_ptr<TimeOutTermOptions>(new TimeOutTermOptions{1000000, 1});

    //// Mesh & Viewscreen ////
    // Mesh possibilities & syntax:
    // SimpleSquareMesh(largecounter totalPixels, ValDiag);
    // InputCertainPixelsMesh(largecounter totalPixels, ValDiag); // totalPixels fixes the screen size in pixels, pixels to be integrated will be user-inputted
    // SquareSubdivisionMesh: see below
    std::unique_ptr<Mesh> theMesh = std::unique_ptr<Mesh>(new SquareSubdivisionMesh(
        0,         // maxpixels (0 = infinite)
        10000,     // initial pixels
        7,         // maxsubdivide
        2000,      // iteration pixels
        false,     // initial subdivide to final
        ValDiag)); // value diagnostic to be used for calculating distances

    // ViewScreen syntax: see below
    theView = std::unique_ptr<ViewScreen>(new ViewScreen(
        {0.0, 1000.0, 0.2966972222222, 0.0}, // position
        {0.0, -1.0, 0.0, 0.0},               // direction
        {15, 15},                            // screen size
        {0, 0},                              // screen center
        std::move(theMesh),                  // R-value of Mesh --- ViewScreen becomes owner!
        theM.get()));                        // (non-owner) pointer to Metric

    //// Integrator ////
    theIntegrator = Integrators::IntegrateGeodesicStep_RK4; // IntegrateGeodesicStep_RK4 or IntegrateGeodesicStep_Verlet
    Integrators::IntegratorDescription = "RK4";
    Integrators::epsilon = 0.03; // base step size that is used (is adapted dynamically)

    //// Output handler ////
    // Syntax: see below
    theOutputHandler = std::unique_ptr<GeodesicOutputHandler>(new GeodesicOutputHandler(
        "output",                                                                                             // file prefix
        Utilities::GetTimeStampString(),                                                                      // time stamp
        "dat",                                                                                                // file extension
        Utilities::GetDiagNameStrings(AllDiags, ValDiag),                                                     // strings of names of all Diagnostics turned on
        200000,                                                                                               // nr geodesics to cache
        200000,                                                                                               // nr geodesics per file
        Utilities::GetFirstLineInfoString(theM.get(), theS.get(), AllDiags, ValDiag, AllTerms, theView.get()) // first line info
        ));
}

// in main itself

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
