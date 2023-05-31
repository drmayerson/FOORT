#include "Config.h" // We are implementing these Config namespace functions here

#include "Utilities.h" // for Utilities::GetDiagNameStrings

#include <algorithm> // for std::transform, std::max, std::min
#include <cctype> // for std::to_lower
#include <utility> // std::move


// DECLARATION OF ALL static DiagnosticOptions (for all types of Diagnostics) needed here!
std::unique_ptr<GeodesicPositionOptions> GeodesicPositionDiagnostic::DiagOptions;
std::unique_ptr<EquatorialPassesOptions> EquatorialPassesDiagnostic::DiagOptions;
std::unique_ptr<ClosestRadiusOptions> ClosestRadiusDiagnostic::DiagOptions;
std::unique_ptr<EquatorialEmissionOptions> EquatorialEmissionDiagnostic::DiagOptions;


//// DIAGNOSTIC ADD POINT D.1 ////
// Declare your Diagnostic's static DiagnosticOptions struct here!
// Sample code:
/*
std::unique_ptr<DiagnosticOptions> MyDiagnostic::DiagOptions; // change DiagnosticOptions to your descendant struct if necessary
*/
//// END DIAGNOSTIC ADD POINT D.1 ////


// DECLARATION OF ALL static TerminationOptions (for all types of Terminations) needed here!
std::unique_ptr<HorizonTermOptions> HorizonTermination::TermOptions;
std::unique_ptr<BoundarySphereTermOptions> BoundarySphereTermination::TermOptions;
std::unique_ptr<TimeOutTermOptions> TimeOutTermination::TermOptions;
std::unique_ptr<ThetaSingularityTermOptions> ThetaSingularityTermination::TermOptions;
std::unique_ptr<NaNTermOptions> NaNTermination::TermOptions;
std::unique_ptr<GeneralSingularityTermOptions> GeneralSingularityTermination::TermOptions;

//// TERMINATION ADD POINT D.1 ////
// Declare your Termination's static TerminationOptions struct here!
// Sample code:
/*
std::unique_ptr<MyTermOptions> MyTermination::DiagOptions;
*/
//// END TERMINATION ADD POINT D.1 ////


///////////////////////////////////////////////////////////////////////////////////
// Config namespace and all of its functions are only defined in CONFIGURATION_MODE
#ifdef CONFIGURATION_MODE


// Initialize screen output options
void Config::InitializeScreenOutput(const ConfigCollection& theCfg)
{
	// DEFAULT: highest level output allowed
	SetOutputLevel(OutputLevel::Level_4_DEBUG);

	if (theCfg.Exists("Output"))
	{
		const ConfigCollection& OutputSettings = theCfg["Output"];

		// Screen output level (overall)
		int scroutputint{ static_cast<int>(OutputLevel::Level_4_DEBUG) };
		OutputSettings.LookupValueInteger("ScreenOutputLevel", scroutputint);
		// We do these checks to make sure the int we have read in can indeed be interpreted as an OutputLevel
		scroutputint = std::max(scroutputint, static_cast<int>(OutputLevel::Level_0_WARNING));
		scroutputint = std::min(scroutputint, static_cast<int>(OutputLevel::MaxLevel));
		SetOutputLevel(static_cast<OutputLevel>(scroutputint));

		// Retrieve loop message frequency (within every integration loop)
		largecounter loopmessagefrequency{ GetLoopMessageFrequency() };
		OutputSettings.LookupValueInteger("LoopMessageFrequency", loopmessagefrequency);
		SetLoopMessageFrequency(loopmessagefrequency);
	}
}


/// <summary>
/// Config::GetMetric():  Use configuration to create the correct Metric with specified parameters
/// </summary>
std::unique_ptr<Metric> Config::GetMetric(const ConfigCollection& theCfg)
{
	std::string MetricName{};

	// DEFAULT: Kerr with a = 0.5, epsHorizon = 0.01
	std::unique_ptr<Metric> TheMetric{ new KerrMetric{0.5} };
	std::string DefaultString{ "Kerr with a = 0.5" };

	try
	{
		// Check to see that there are Metric settings at all
		if (!theCfg.Exists("Metric"))
		{
			throw SettingError("No metric settings found.");
		}

		// Go to the Metric settings
		const ConfigCollection& MetricSettings = theCfg["Metric"];

		// Check log r setting first
		bool rLogScale{ false };
		MetricSettings.LookupValue("RLogScale", rLogScale);

		// Check to see that the Metric's name has been specified
		if (!MetricSettings.LookupValue("Name", MetricName))
		{
			throw SettingError("No metric settings found.");
		}

		// Metric name exists; now we can look up the correct metric
		// Make MetricName all lower case to avoid case-specific errors
		std::transform(MetricName.begin(), MetricName.end(), MetricName.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		// COMPARISON WITH ALL LOWER CASE LETTERS!
		if (MetricName == "kerr")
		{
			// Kerr
			
			// First setting to look up: the a parameter
			double theKerra{ 0.5 };
			if (!MetricSettings.LookupValue("a", theKerra))
			{
				ScreenOutput("Kerr: no value for a given. Using default: " + std::to_string(theKerra) + ".",
					Output_Other_Default);
			}


			// All settings complete; create Metric object!
			TheMetric = std::unique_ptr<Metric>(new KerrMetric( theKerra, rLogScale ));
		}
		else if (MetricName == "flatspace")
		{
			// (4D) flat space in spherical coordinates; no options necessary
			// Create Metric object!
			TheMetric = std::unique_ptr<Metric>(new FlatSpaceMetric(rLogScale));
		}
		else if (MetricName == "rasheedlarsen" || MetricName == "rasheed-larsen")
		{
			// The Rasheed-Larsen black hole
			
			// Look up four parameters of BH
			double RLm{ 1.0 };
			double RLa{ 0.5 };
			double RLp{ 2.0 };
			double RLq{ 2.0 };
			if (!MetricSettings.LookupValue("m", RLm))
			{
				ScreenOutput("Rasheed-Larsen: no value for m given. Using default: " + std::to_string(RLm) + ".",
					Output_Other_Default);
			}
			if (!MetricSettings.LookupValue("a", RLa))
			{
				ScreenOutput("Rasheed-Larsen: no value for a given. Using default: " + std::to_string(RLa) + ".",
					Output_Other_Default);
			}
			if (!MetricSettings.LookupValue("p", RLp))
			{
				ScreenOutput("Rasheed-Larsen: no value for p given. Using default: " + std::to_string(RLp) + ".",
					Output_Other_Default);
			}
			if (!MetricSettings.LookupValue("q", RLq))
			{
				ScreenOutput("Rasheed-Larsen: no value for q given. Using default: " + std::to_string(RLq) + ".",
					Output_Other_Default);
			}

			// All settings complete; create Metric object!
			TheMetric = std::unique_ptr<Metric>(new RasheedLarsenMetric(RLm, RLa, RLp, RLq, rLogScale ) );
		}
		else if (MetricName == "johannsen")
		{
			// The Johannsen black hole (implementation by Seppe Staelens)

			// Look up five parameters of BH
			double JOHa{ 0.7 };
			double JOHa13{ 2.0 };
			double JOHa22{ 0. };
			double JOHa52{ 0. };
			double JOHe3{ 0. };
			if (!MetricSettings.LookupValue("a", JOHa))
			{
				ScreenOutput("Johannsen: no value for a given. Using default: " + std::to_string(JOHa) + ".",
					Output_Other_Default);
			}
			if (!MetricSettings.LookupValue("alpha13", JOHa13))
			{
				ScreenOutput("Johannsen: no value for alpha13 given. Using default: " + std::to_string(JOHa13) + ".",
					Output_Other_Default);
			}
			if (!MetricSettings.LookupValue("alpha22", JOHa22))
			{
				ScreenOutput("Johannsen: no value for alpha22 given. Using default: " + std::to_string(JOHa22) + ".",
					Output_Other_Default);
			}
			if (!MetricSettings.LookupValue("alpha52", JOHa52))
			{
				ScreenOutput("Johannsen: no value for alpha52 given. Using default: " + std::to_string(JOHa52) + ".",
					Output_Other_Default);
			}
			if (!MetricSettings.LookupValue("epsilon3", JOHe3))
			{
				ScreenOutput("Johannsen: no value for epsilon3 given. Using default: " + std::to_string(JOHe3) + ".",
					Output_Other_Default);
			}

			// All settings complete; create Metric object!
			TheMetric = std::unique_ptr<Metric>(new JohannsenMetric(JOHa, JOHa13, JOHa22, JOHa52, JOHe3, rLogScale));
		}
		else if (MetricName == "mankonovikov" || MetricName == "manko-novikov")
		{
			// The Manko-Novikov black hole with alpha3

			// Look up two parameters of BH
			double MNa{ 0. };
			double MNalpha3{ 5.0 };
			if (!MetricSettings.LookupValue("a", MNa))
			{
				ScreenOutput("Manko-Novikov: no value for a given. Using default: " + std::to_string(MNa) + ".",
					Output_Other_Default);
			}
			if (!MetricSettings.LookupValue("alpha3", MNalpha3))
			{
				ScreenOutput("Manko-Novikov: no value for alpha3 given. Using default: " + std::to_string(MNalpha3) + ".",
					Output_Other_Default);
			}

			// All settings complete; create Metric object!
			TheMetric = std::unique_ptr<Metric>(new MankoNovikovMetric(MNa, MNalpha3, rLogScale));
		}
		else if (MetricName == "kerrschild" || MetricName == "kerr-schild")
		{
			// Kerr-Schild

			// First setting to look up: the a parameter
			double theKerra{ 0.5 };
			if (!MetricSettings.LookupValue("a", theKerra))
			{
				ScreenOutput("Kerr-Schild: no value for a given. Using default: " + std::to_string(theKerra) + ".",
					Output_Other_Default);
			}

			// All settings complete; create Metric object!
			TheMetric = std::unique_ptr<Metric>(new KerrSchildMetric{ theKerra, rLogScale });
		}
		else if (MetricName == "st3cr")
		{
			// Ring fuzzball

			real ST3CrP{ 2. };
			real ST3Crq0{ 50. };
			real ST3Crlambda{ 0.19 };

			if (!MetricSettings.LookupValue("P", ST3CrP))
			{
				ScreenOutput("ST3Cr: no value for P given. Using default: " + std::to_string(ST3CrP) + ".",
					Output_Other_Default);
			}
			if (!MetricSettings.LookupValue("q0", ST3Crq0))
			{
				ScreenOutput("ST3Cr: no value for q0 given. Using default: " + std::to_string(ST3Crq0) + ".",
					Output_Other_Default);
			}
			if (!MetricSettings.LookupValue("lambda", ST3Crlambda))
			{
				ScreenOutput("ST3Cr: no value for lambda given. Using default: " + std::to_string(ST3Crlambda) + ".",
					Output_Other_Default);
			}

			TheMetric = std::unique_ptr<Metric>(new ST3CrMetric( ST3CrP, ST3Crq0, ST3Crlambda, rLogScale ));
		}
		//// METRIC ADD POINT B ////
		// Add an else if clause to check for your new Metric object!
		// To look for additional options in the metric configuration, use
		// MetricSettings.LookupValue("OptionName", optionvar);
		// Sample code:
		/*
		else if (MetricName == "mymetric") // remember to use all lower case!
		{
			type myparam{ defaultsetting }; 
			// Note: if this option is not present in the configuration file, then myparam will not be changed
			// by the call to LookupValue
			MetricSettings.LookupValue("MyParameter", myparam);

			TheMetric = std::unique_ptr<Metric>(new MyMetric{ myparam }); // put your additional parameters needed in the constructor
		}
		*/
		//// END METRIC ADD POINT B ////
		else // no match found: must be incorrect metric name specified in configuration file
		{
			throw SettingError("No metric settings found.");
		}
	}
	catch (SettingError& e)
	{
		// Something happened so that we were unable to determine even which metric to use.
		// Use the default settings (set above at begin of this function)
		ScreenOutput(std::string(e.what()) + " Using default metric (" + DefaultString + ").",
			Output_Important_Default);
	}

	return TheMetric;
}


/// <summary>
/// Config::GetSource():  Use configuration to create the correct Source with specified parameters
/// </summary>
std::unique_ptr<Source> Config::GetSource(const ConfigCollection& theCfg, const Metric* const theMetric)
{
	std::string SourceName{};

	// SET DEFAULTS HERE: no source
	std::unique_ptr<Source> TheSource{ new NoSource(theMetric) };
	std::string DefaultString{ "No source." };

	try
	{
		// Check to see that there are Source settings at all
		if (!theCfg.Exists("Source"))
		{
			throw SettingError("No geodesic source settings found.");
		}

		// Go to the Source settings
		const ConfigCollection& SourceSettings = theCfg["Source"];

		// Check to see that the Source's name has been specified
		if (!SourceSettings.LookupValue("Name", SourceName))
		{
			throw SettingError("No source settings found.");
		}

		// Source name exists; now we can look up the correct source
		// Make SourceName all lower case to avoid case-specific errors
		std::transform(SourceName.begin(), SourceName.end(), SourceName.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		// COMPARISON WITH ALL LOWER CASE LETTERS!
		if (SourceName == "nosource")
		{
			// No further settings needed; create Source!
			TheSource = std::unique_ptr<Source>(new NoSource(theMetric));
		}
		// else if ... (other sources here)
		else // no match found: must be incorrect source name specified in configuration file
		{
			throw SettingError("No source settings found.");
		}
	}
	catch (SettingError& e)
	{
		// Something happened so that we were unable to determine even which source to use.
		// Use the default settings (set above at begin of this function)
		// This is not so important since it is just the source --- we can assume this is just NoSource by default
		ScreenOutput(std::string(e.what()) + " Using default source (" + DefaultString + ").",
			Output_Other_Default);
	}

	return TheSource;
}



/// <summary>
/// Config::InitializeDiagnostics():  Use configuration to set the Diagnostics bitflag appropriately;
/// initialize all DiagnosticOptions for all Diagnostics that are turned on;
/// and set bitflag for diagnostic to be used for coarseness evaluating in Mesh
/// </summary>
void Config::InitializeDiagnostics(const ConfigCollection& theCfg, DiagBitflag& alldiags, DiagBitflag& valdiag, const Metric* const theMetric)
{
	// First set these flags to all zeros
	alldiags = Diag_None;
	valdiag = Diag_None;


	try
	{
		// Check to see that there are Diagnostics settings at all
		if (!theCfg.Exists("Diagnostics"))
		{
			throw SettingError("No diagnostic settings found.");
		}

		// Go to the Diagnostics settings
		const ConfigCollection& AllDiagSettings = theCfg["Diagnostics"];

		// Helper function: checks to see if a particular Diagnostic (with name Diagname)
		// is present in the Diagnostics options; if the setting "on" is specified;
		// and if "on" is also set to "true" (so by default "on" = false)
		// Only if all these are satisfied do we want to have the Diagnostic included (and initialized)
		auto CheckIfDiagOn = [&AllDiagSettings](const std::string& DiagName)
		{
			bool diagison{ false };
			return AllDiagSettings.Exists(DiagName.c_str()) // note that g++ on Fabio's Linux requires the .c_str() everywhere
				&& AllDiagSettings[DiagName.c_str()].LookupValue("On", diagison)
				&& diagison;
		};

		// FourColorScreen
		if (CheckIfDiagOn("FourColorScreen"))
		{
			// Four color screen diagnostic on! Add it to bitflag.
			alldiags |= Diag_FourColorScreen;

			// There are no options to set for FourColorScreen

			// check to see if there is no valdiag set yet;
			// and there is a option "UseForMesh" specied for this Diagnostic;
			// and "UseForMesh" is set to true
			bool isVal{ false };
			if (valdiag == Diag_None && AllDiagSettings["FourColorScreen"].LookupValue("UseForMesh", isVal) && isVal)
			{
				// Use this Diagnostic for the Mesh values
				valdiag = Diag_FourColorScreen;
			}
		}
		// GeodesicPosition
		if (CheckIfDiagOn("GeodesicPosition"))
		{
			alldiags |= Diag_GeodesicPosition;

			largecounter updatensteps = 1;
			bool updatestart{ false };
			bool updatefinish{ true };
			AllDiagSettings["GeodesicPosition"].LookupValueInteger("UpdateFrequency", updatensteps);
			if (updatensteps == 0)
			{
				AllDiagSettings["GeodesicPosition"].LookupValue("UpdateStart", updatestart);
				AllDiagSettings["GeodesicPosition"].LookupValue("UpdateFinish", updatefinish);
			}

			largecounter outputsteps = 0; // keep all steps
			AllDiagSettings["GeodesicPosition"].LookupValueInteger("OutputSteps", outputsteps);

			GeodesicPositionDiagnostic::DiagOptions = 
				std::unique_ptr<GeodesicPositionOptions>(new GeodesicPositionOptions{ outputsteps,
											UpdateFrequency{updatensteps,updatestart,updatefinish} });

			bool isVal{ false };
			if (valdiag == Diag_None && AllDiagSettings["GeodesicPosition"].LookupValue("UseForMesh", isVal) && isVal)
			{
				// Use this Diagnostic for the Mesh values
				valdiag = Diag_GeodesicPosition;
			}
		}
		// GeodesicPosition
		if (CheckIfDiagOn("EquatorialPasses"))
		{
			alldiags |= Diag_EquatorialPasses;

			largecounter updatensteps = 1;
			bool updatestart{ true };
			bool updatefinish{ true };
			AllDiagSettings["EquatorialPasses"].LookupValueInteger("UpdateFrequency", updatensteps);
			if (updatensteps == 0)
			{
				AllDiagSettings["EquatorialPasses"].LookupValue("UpdateStart", updatestart);
				AllDiagSettings["EquatorialPasses"].LookupValue("UpdateFinish", updatefinish);
			}

			real threshold{ 0.01 };
			AllDiagSettings["EquatorialPasses"].LookupValue("Threshold", threshold);


			EquatorialPassesDiagnostic::DiagOptions =
				std::unique_ptr<EquatorialPassesOptions>(new EquatorialPassesOptions( threshold, UpdateFrequency{updatensteps,
					updatestart,updatefinish} ));

			bool isVal{ false };
			if (valdiag == Diag_None && AllDiagSettings["EquatorialPasses"].LookupValue("UseForMesh", isVal) && isVal)
			{
				// Use this Diagnostic for the Mesh values
				valdiag = Diag_EquatorialPasses;
			}
		}
		// ClosestRadius
		if (CheckIfDiagOn("ClosestRadius"))
		{
			alldiags |= Diag_ClosestRadius;

			largecounter updatensteps = 1;
			bool updatestart{ true };
			bool updatefinish{ true };
			AllDiagSettings["ClosestRadius"].LookupValueInteger("UpdateFrequency", updatensteps);
			if (updatensteps == 0)
			{
				AllDiagSettings["ClosestRadius"].LookupValue("UpdateStart", updatestart);
				AllDiagSettings["ClosestRadius"].LookupValue("UpdateFinish", updatefinish);
			}

			// Check if metric is a spherical horizon metric with integration set to a logarithmic r coordinate,
			// if so, pass this along to the options struct
			bool rlogradius{ theMetric->getrLogScale()};

			ClosestRadiusDiagnostic::DiagOptions =
				std::unique_ptr<ClosestRadiusOptions>(new ClosestRadiusOptions(rlogradius, UpdateFrequency{ updatensteps,
					updatestart,updatefinish }));

			bool isVal{ false };
			if (valdiag == Diag_None && AllDiagSettings["ClosestRadius"].LookupValue("UseForMesh", isVal) && isVal)
			{
				// Use this Diagnostic for the Mesh values
				valdiag = Diag_ClosestRadius;
			}
		}
		// EquatorialEmission
		if (CheckIfDiagOn("EquatorialEmission"))
		{
			alldiags |= Diag_EquatorialEmission;

			// We don't want EquatorialPasses to be on! It is superfluous (EquatorialEmission also keeps track of equatorial passes
			// and this would give issues with the EquatorialPassesOptions struct (see below)
			if (alldiags & Diag_EquatorialPasses)
			{
				ScreenOutput("Configuration indicates equatorial emission and equatorial passes turned on. Turning off equatorial passes.",
					Output_Important_Default);
				alldiags = alldiags & ~Diag_EquatorialPasses;
				if (valdiag & Diag_EquatorialPasses)
				{
					ScreenOutput("Changing value mesh diagnostic to equatorial emission instead of equatorial passes.",
						Output_Important_Default);
					valdiag = Diag_EquatorialEmission;
				}
			}

			// Basic frequency settings
			largecounter updatensteps = 1;
			bool updatestart{ true };
			bool updatefinish{ true };
			AllDiagSettings["EquatorialEmission"].LookupValueInteger("UpdateFrequency", updatensteps);
			if (updatensteps == 0)
			{
				AllDiagSettings["EquatorialEmission"].LookupValue("UpdateStart", updatestart);
				AllDiagSettings["EquatorialEmission"].LookupValue("UpdateFinish", updatefinish);
			}

			// Check if metric is a spherical horizon metric with integration set to a logarithmic r coordinate,
			// if so, pass this along to the options struct
			bool rlogradius{ theMetric->getrLogScale() };

			// Looking up overall options for emission

			real threshold{ 0.01 };
			AllDiagSettings["EquatorialEmission"].LookupValue("Threshold", threshold);

			real fudgefactor{ 1.0 };
			AllDiagSettings["EquatorialEmission"].LookupValue("GeometricFudgeFactor", fudgefactor);

			int equatupper{ 0 };
			AllDiagSettings["EquatorialEmission"].LookupValue("EquatPassUpperBound", equatupper);

			int redshiftpower{ 3 };
			AllDiagSettings["EquatorialEmission"].LookupValue("RedshiftPower", redshiftpower);


			//// Emission model selection and initialization ////

			// Default emission model and parameters
			const SphericalHorizonMetric* sphermetric{ dynamic_cast<const SphericalHorizonMetric*>(theMetric) };
			real defaultmu{ sphermetric ? sphermetric->getHorizonRadius() : 1.0 };
			real defaultgamma{ 0.0 };
			real defaultsigma{ 1.0 };
			std::unique_ptr<EmissionModel> theEmission{ new GLMJohnsonSUEmission(defaultmu, defaultgamma, defaultsigma) };

			// Read in emission model
			std::string emitmodelstring{ "" };
			AllDiagSettings["EquatorialEmission"].LookupValue("EmissionModel", emitmodelstring);
			if (emitmodelstring == "GLMJohnsonSU")
			{
				real mu{ defaultmu };
				real gamma{ defaultgamma };
				real sigma{ defaultsigma };
				AllDiagSettings["EquatorialEmission"].LookupValue("mu", mu);
				AllDiagSettings["EquatorialEmission"].LookupValue("gamma", gamma);
				AllDiagSettings["EquatorialEmission"].LookupValue("sigma", sigma);
				theEmission = std::unique_ptr<EmissionModel>{ new GLMJohnsonSUEmission(mu, gamma, sigma) };
			}
			// Other emission models can be checked for here...


			//// Fluid four-velocity model selection and initialization ////

			// Default fluid model and default parameters
			real defaultxi{ 1.0 };
			real defaultbetar{ 1.0 };
			real defaultbetaphi{ 1.0 };
			std::unique_ptr<FluidVelocityModel> theFluidModel{ new GeneralCircularRadialFluid(defaultxi, defaultbetar, defaultbetaphi, theMetric) };

			// Read in fluid velocity model
			std::string fluidmodelstring{ "" };
			AllDiagSettings["EquatorialEmission"].LookupValue("FluidVelocityModel", fluidmodelstring);
			if (fluidmodelstring == "GeneralCircularRadial")
			{
				real subKeplerianparam{ defaultxi };
				real betaR{ defaultbetar };
				real betaPhi{ defaultbetaphi };
				AllDiagSettings["EquatorialEmission"].LookupValue("xi", subKeplerianparam);
				AllDiagSettings["EquatorialEmission"].LookupValue("betar", betaR);
				AllDiagSettings["EquatorialEmission"].LookupValue("betaphi", betaPhi);
				
				theFluidModel = std::unique_ptr<FluidVelocityModel>{ new GeneralCircularRadialFluid(subKeplerianparam,betaR,betaPhi,theMetric) };
			}
			// Other fluid velocity models can be checked for here...


			// Set EquatorialEmissionDiagnostic options struct
			EquatorialEmissionDiagnostic::DiagOptions =
				std::unique_ptr<EquatorialEmissionOptions>(new EquatorialEmissionOptions(
					fudgefactor, equatupper, std::move(theEmission), std::move(theFluidModel),
					rlogradius, redshiftpower, threshold, UpdateFrequency{ updatensteps,updatestart,updatefinish } ));

			// We also need to set EquatorialPassesDiagnostic options struct correctly!
			// EquatorialEmissionDiagnostic::UpdateData() calls its base class
			// implementation EquatorialPassesDiagnostic::UpdateData(), which uses this struct
			EquatorialPassesDiagnostic::DiagOptions =
				std::unique_ptr<EquatorialPassesOptions>(new EquatorialPassesOptions(threshold, UpdateFrequency{ updatensteps,
					updatestart,updatefinish }));

			bool isVal{ false };
			if (valdiag == Diag_None && AllDiagSettings["EquatorialEmission"].LookupValue("UseForMesh", isVal) && isVal)
			{
				// Use this Diagnostic for the Mesh values
				valdiag = Diag_EquatorialEmission;
			}
		}

		//// DIAGNOSTIC ADD POINT D.2 ////
		// Add a check to see if your new Diagnostic is turned on here. If it is, check any further options it needs and
		// make sure to set the diagnostic flags appropriately.
		// Sample code:
		/*
		// MyDiagnostic
		if (CheckIfDiagOn("MyDiagnostic"))
		{
			alldiags |= Diag_MyDiagnostic; // use the flag you created at DIAGNOSTIC ADD POINT B.

			// The following assumes your new Diagnostic carries a static DiagnosticOptions struct.
			// If it does not, or if it carries additional options (in a descendant struct of DiagnosticOptions),
			// then update this accordingly
			largecounter updatensteps = 1;
			bool updatestart{ true }; 
			bool updatefinish{ true };
			AllDiagSettings["MyDiagnostic"].LookupValueInteger("UpdateFrequency", updatensteps);
			if (updatensteps == 0)
			{
				AllDiagSettings["MyDiagnostic"].LookupValue("UpdateStart", updatestart);
				AllDiagSettings["MyDiagnostic"].LookupValue("UpdateFinish", updatefinish);
			}
			// (look up any additional options here...)

			MyDiagnostic::DiagOptions =
				std::unique_ptr<DiagnosticOptions>(new DiagnosticOptions{ UpdateFrequency{updatensteps,
				updatestart,updatefinish} });

			bool isVal{ false };
			if (valdiag == Diag_None && AllDiagSettings["MyDiagnostic"].LookupValue("UseForMesh", isVal) && isVal)
			{
				// Use this Diagnostic for the Mesh values
				valdiag = Diag_MyDiagnostic; // use the flag you created at DIAGNOSTIC ADD POINT B.
			}
		}
		*/
		//// END DIAGNOSTIC ADD POINT D.2. ////



		// Done looking for diagnostics. Make sure something has been turned on!
		if (alldiags == Diag_None)
		{
			throw SettingError("No diagnostics turned on.");
		}

		// Make sure something has been specified as the value Diagnostic (for the Mesh)
		// If not, then set it to the FourColorScreen Diagnostic by default if FourColorScreen is present
		// If also FourColorScreen is not turned on, then revert to all defaults for Diagnostics.
		if (valdiag == Diag_None)
		{
			if (alldiags & Diag_FourColorScreen)
			{
				ScreenOutput("No mesh diagnostic set; using FourColorScreen.", Output_Other_Default);
				valdiag = Diag_FourColorScreen;
			}
			else
			{
				throw SettingError("Diagnostics turned on but no Mesh diagnostic selected, and FourColorScreen not turned on.");
			}
		}
	}
	catch (SettingError& e)
	{
		// Something happened (e.g. no Diagnostic options specified) that makes us revert completely to
		// default Diagnostic options (which are SET HERE)
		ScreenOutput(std::string(e.what()) + " Using default diagnostic(s) (" + "FourColorScreen" + ").",
			Output_Important_Default);
		alldiags = Diag_FourColorScreen;
		valdiag = Diag_FourColorScreen;
	}

}


/// <summary>
/// Config::InitializeTerminations():  Use configuration to set the Termination bitflag appropriately;
/// and initialize all TerminationOptions for all Terminations that are turned on.
/// </summary>
void Config::InitializeTerminations(const ConfigCollection& theCfg, TermBitflag& allterms, const Metric* const theMetric)
{
	// First set the flag to all zeros
	allterms = Term_None;

	try
	{
		// Check to see that there are Terminations settings at all
		if (!theCfg.Exists("Terminations"))
		{
			throw SettingError("No termination settings found.");
		}

		// Go to the Terminations settings
		const ConfigCollection& AllTermSettings = theCfg["Terminations"];

		// Helper function: checks to see if a particular Termination (with name TermName)
		// is present in the Terminations options; if the setting "on" is specified;
		// and if "on" is also set to "true" (so by default "on" = false)
		// Only if all these are satisfied do we want to have the Termination included (and initialized)
		auto CheckIfTermOn = [&AllTermSettings](const std::string& TermName)
		{
			bool termison{ false };
			return AllTermSettings.Exists(TermName.c_str()) // note that g++ on Fabio's Linux requires the .c_str() everywhere
				&& AllTermSettings[TermName.c_str()].LookupValue("On", termison)
				&& termison;
		};

		if (CheckIfTermOn("Horizon"))
		{
			
			// Make sure metric is of the horizon type!
			const SphericalHorizonMetric* sphermetric = dynamic_cast<const SphericalHorizonMetric*>(theMetric);
			if (!sphermetric)
			{
				ScreenOutput("Horizon Termination turned on but metric does not have horizon! Turning off Horizon Termination.",
					Output_Important_Default);
			}
			else
			{
				// Horizon
				allterms |= Term_Horizon;

				// Look up horizon radius and r log scale
				real horizonRadius = sphermetric->getHorizonRadius();
				bool rLogScale = sphermetric->getrLogScale();

				// Setting to look up: (relative) radial distance to horizon before stopping integration
				double epsHorizon{ 0.01 };
				AllTermSettings["Horizon"].LookupValue("Epsilon_Horizon", epsHorizon);

				// By default, this Termination updates every step
				// Check to see if a different update frequency has been specified
				largecounter updatefreq = 1;
				AllTermSettings["Horizon"].LookupValueInteger("UpdateFrequency", updatefreq);

				// Initialize the (static) TerminationOptions for Horizon!
				HorizonTermination::TermOptions =
					std::unique_ptr<HorizonTermOptions>(new HorizonTermOptions{ horizonRadius,rLogScale,epsHorizon,
					updatefreq });
			}
		}

		// BoundarySphere
		if (CheckIfTermOn("BoundarySphere"))
		{
			// Boundary sphere termination on! Add it to bitflag.
			allterms |= Term_BoundarySphere;

			// Get the radius of the boundary sphere. Default is 1000(M).
			real radius{ 1000 };
			AllTermSettings["BoundarySphere"].LookupValue("SphereRadius", radius);

			// Check if metric is a spherical horizon metric with integration set to a logarithmic r coordinate,
			// if so, pass this along to the options struct
			bool rlogradius{ theMetric->getrLogScale() };

			// By default, this Termination updates every step
			// Check to see if a different update frequency has been specified
			largecounter updatefreq = 1;
			AllTermSettings["BoundarySphere"].LookupValueInteger("UpdateFrequency", updatefreq);

			// Initialize the (static) TerminationOptions for BoundarySphere!
			BoundarySphereTermination::TermOptions =
				std::unique_ptr<BoundarySphereTermOptions>(new BoundarySphereTermOptions{ radius, rlogradius, updatefreq });
		}

		// TimeOut
		if (CheckIfTermOn("TimeOut"))
		{
			// Time out termination on! Add it to bitflag.
			allterms |= Term_TimeOut;

			// Get the number of steps until time-out. Default is 10000
			largecounter timeoutsteps { 10000 };
			AllTermSettings["TimeOut"].LookupValueInteger("MaxSteps", timeoutsteps);

			// By default, this Termination updates every step
			// Check to see if a different update frequency has been specified
			largecounter updatefreq = 1;
			AllTermSettings["TimeOut"].LookupValueInteger("UpdateFrequency", updatefreq);

			// Initialize the (static) TerminationOptions for TimeOut!
			TimeOutTermination::TermOptions =
				std::unique_ptr<TimeOutTermOptions>(new TimeOutTermOptions{ timeoutsteps,
					updatefreq });
		}

		// ThetaSingularity
		if (CheckIfTermOn("ThetaSingularity"))
		{
			// Theta singularity check on! Add to bitflag
			allterms |= Term_ThetaSingularity;

			// get the tolerance for how much theta is allowed to get near the poles; default 1e-5
			real epsilon{ 1e-5 };
			AllTermSettings["ThetaSingularity"].LookupValue("Epsilon", epsilon);

			// By default, this Termination updates every step
			// Check to see if a different update frequency has been specified
			largecounter updatefreq = 1;
			AllTermSettings["ThetaSingularity"].LookupValueInteger("UpdateFrequency", updatefreq);

			// Initialize the (static) TerminationOptions for ThetaSingularity!
			ThetaSingularityTermination::TermOptions =
				std::unique_ptr<ThetaSingularityTermOptions>(new ThetaSingularityTermOptions{ epsilon, updatefreq });
		}

		// NaN
		if (CheckIfTermOn("NaN"))
		{
			// Theta singularity check on! Add to bitflag
			allterms |= Term_NaN;

			// output information of geodesic that hits a nan or not
			bool outputconsole{ true };
			AllTermSettings["NaN"].LookupValue("ConsoleOutput", outputconsole);


			// By default, this Termination updates every step
			// Check to see if a different update frequency has been specified
			largecounter updatefreq = 1;
			AllTermSettings["NaN"].LookupValueInteger("UpdateFrequency", updatefreq);

			// Initialize the (static) TerminationOptions for NaN!
			NaNTermination::TermOptions =
				std::unique_ptr<NaNTermOptions>(new NaNTermOptions( outputconsole, updatefreq ));
		}

		// General singularities
		if (CheckIfTermOn("GeneralSingularity"))
		{
			// Make sure metric is of the singularity type!
			const SingularityMetric* singmetric = dynamic_cast<const SingularityMetric*>(theMetric);
			if (!singmetric)
			{
				ScreenOutput("General singularity Termination turned on but metric does not have singularities! Turning off General singularity Termination.",
					Output_Important_Default);
			}
			else
			{
				// General singularities on! Add to bitflag
				allterms |= Term_GeneralSingularity;

				std::vector<Singularity> thesings{ singmetric->getSingularities() };

				bool rLogScale = singmetric->getrLogScale();

				// get the tolerance for how close we are allowed to get (in coordinates) to the singularity
				real epsilon{ 1e-3 };
				AllTermSettings["GeneralSingularity"].LookupValue("Epsilon", epsilon);

				// output information of geodesic that hits a nan or not
				bool outputconsole{ false };
				AllTermSettings["GeneralSingularity"].LookupValue("ConsoleOutput", outputconsole);

				// By default, this Termination updates every step
				// Check to see if a different update frequency has been specified
				largecounter updatefreq = 1;
				AllTermSettings["GeneralSingularity"].LookupValueInteger("UpdateFrequency", updatefreq);

				// Initialize the (static) TerminationOptions for GeneralSingularity!
				GeneralSingularityTermination::TermOptions =
					std::unique_ptr<GeneralSingularityTermOptions>(new GeneralSingularityTermOptions(std::move(thesings),
						epsilon, outputconsole, rLogScale, updatefreq));
			}
		}

		//// TERMINATION ADD POINT D.2. ////
		// Check to see if your new Termination has been turned on, and if so, add it to the allterms bitflag
		// and set its options accordingly.
		// Sample code:
		/*
		// MyTermination
		if (CheckIfTermOn("MyTermination"))
		{
			allterms |= Term_MyTermination; // use the bitflag you defined at TERMINATION ADD POINT B.1.

			// The following assumes that MyTermination has a static TerminationOptions struct,
			// if it does not or has additional options (i.e. it has a descendant of TerminationOptions as its
			// static struct), then update here accordingly.
			largecounter updatefreq = 1;
			AllTermSettings["MyTermination"].LookupValueInteger("UpdateFrequency", updatefreq);

			// Initialize the (static) TerminationOptions!
			MyTermination::TermOptions =
				std::unique_ptr<TerminationOptions>(new TerminationOptions{ updatefreq });
		}
		*/
		//// END TERMINATION ADD POINT D.2. ////


		// Done looking for terminations. Make sure something has been turned on!
		if (allterms == Term_None)
		{
			throw SettingError("No terminations turned on.");
		}
	}
	catch (SettingError& e)
	{
		// Something happened (e.g. no Termination options specified) that makes us revert completely to
		// default Termination options (which are SET HERE)
		ScreenOutput(std::string(e.what()) + " Using default termination(s) (" + "BoundarySphere and TimeOut" + ").",
			Output_Important_Default);
		allterms= Term_BoundarySphere | Term_TimeOut;
		BoundarySphereTermination::TermOptions =
			std::unique_ptr<BoundarySphereTermOptions>(new BoundarySphereTermOptions{ 1000,
				theMetric->getrLogScale(), 1});
		TimeOutTermination::TermOptions =
			std::unique_ptr<TimeOutTermOptions>(new TimeOutTermOptions{ 10000, 1 });
	}

}


/// <summary>
/// Config::GetViewScreen():  Use configuration to create the ViewScreen object;
/// with options set according to the configuration.
/// </summary>
std::unique_ptr<ViewScreen> Config::GetViewScreen(const ConfigCollection& theCfg, DiagBitflag valdiag, const Metric* const theMetric)
{
	// DEFAULTS set here
	Point pos{ 0,1000,pi/2.0,0 };
	OneIndex dir{ 0,-1,0,0 };
	ScreenPoint screensize{ 10.0,10.0 };
	ScreenPoint screencenter{ 0.0, 0.0 };

	try
	{
		// Check to see that there are ViewScreen settings at all
		if (!theCfg.Exists("ViewScreen"))
		{
			throw SettingError("No view screen settings found.");
		}

		// Go to the ViewScreen settings
		const ConfigCollection& ViewSettings = theCfg["ViewScreen"];

		// Look up camera position, direction, screen size
		if (ViewSettings.Exists("Position"))
		{
			ViewSettings["Position"].LookupValue("t", pos[0]);
			ViewSettings["Position"].LookupValue("r", pos[1]);
			ViewSettings["Position"].LookupValue("theta", pos[2]);
			ViewSettings["Position"].LookupValue("phi", pos[3]);
		}
		if (ViewSettings.Exists("Direction"))
		{
			ViewSettings["Direction"].LookupValue("t", dir[0]);
			ViewSettings["Direction"].LookupValue("r", dir[1]);
			ViewSettings["Direction"].LookupValue("theta", dir[2]);
			ViewSettings["Direction"].LookupValue("phi", dir[3]);
		}
		if (ViewSettings.Exists("ScreenSize"))
		{
			ViewSettings["ScreenSize"].LookupValue("x", screensize[0]);
			ViewSettings["ScreenSize"].LookupValue("y", screensize[1]);
		}
		if (ViewSettings.Exists("ScreenCenter"))
		{
			ViewSettings["ScreenCenter"].LookupValue("x", screencenter[0]);
			ViewSettings["ScreenCenter"].LookupValue("y", screencenter[1]);
		}
	}
	catch (SettingError& e)
	{
		ScreenOutput(std::string(e.what()) + " Using default ViewScreen Settings.",
			Output_Important_Default);
	}

	// Get all Mesh settings
	std::unique_ptr<Mesh> theMesh{ Config::GetMesh(theCfg,valdiag)};

	// Create the ViewScreen!
	std::unique_ptr<ViewScreen> theViewScreen{ new ViewScreen(pos, dir, screensize, screencenter,
		std::move(theMesh),theMetric) };

	return theViewScreen;
}


/// <summary>
/// Config::GetMesh():  Use configuration to create the Mesh object;
/// with options set according to the configuration.
/// Config::GetViewScreen() calls this when creating the ViewScreen object.
/// </summary>
std::unique_ptr<Mesh> Config::GetMesh(const ConfigCollection& theCfg, DiagBitflag valdiag)
{
	std::unique_ptr<Mesh> theMesh;

	try
	{
		// Look for the Mesh setting under ViewScreen
		if (!theCfg.Exists("ViewScreen") || !theCfg["ViewScreen"].Exists("Mesh"))
		{
			throw SettingError("No Mesh settings found.");
		}
		const ConfigCollection& MeshSettings = theCfg["ViewScreen"]["Mesh"];

		// Look for the Type option under the Mesh header
		std::string meshname{};
		if (!MeshSettings.LookupValue("Type", meshname))
		{
			throw SettingError("No Mesh Type specified.");
		}

		// We have read in the Mesh type. Now we select and create a Mesh accordingly
		if (meshname == "SimpleSquareMesh")
		{
			// Simple Square Mesh!
			largecounter totalpixels{ 100 * 100 };
			MeshSettings.LookupValueInteger("TotalPixels", totalpixels);

			theMesh = std::unique_ptr<Mesh>(new SimpleSquareMesh(totalpixels, valdiag));
		}
		else if (meshname == "InputCertainPixelsMesh")
		{
			// This Mesh will ask the user to input pixels on the grid specified in the configuration file.
			// (the Mesh will ask for input in its constructor, so here!)
			largecounter totalpixels{ 100 * 100 };
			MeshSettings.LookupValueInteger("TotalPixels", totalpixels);

			theMesh = std::unique_ptr<Mesh>(new InputCertainPixelsMesh(totalpixels, valdiag));
		}
		else if (meshname == "SquareSubdivisionMesh")
		{
			largecounter initialpixels{ 100 };
			largecounter maxpixels{ 100 };
			largecounter iterationpixels{ 100 };
			int maxsubdivide{ 1 };
			bool initialsubtofinal{ false };
			MeshSettings.LookupValueInteger("InitialPixels", initialpixels);
			MeshSettings.LookupValueInteger("MaxPixels", maxpixels);
			MeshSettings.LookupValueInteger("IterationPixels", iterationpixels);
			MeshSettings.LookupValue("MaxSubdivide", maxsubdivide);
			if (maxpixels < initialpixels)
				maxpixels = initialpixels;
			if (maxsubdivide < 1) // 1 is the minimum level (initial grid is level 1)
			{
				ScreenOutput("Invalid MaxSubdivide level given. Using MaxSubdivide = 1.", Output_Other_Default);
				maxsubdivide = 1;
			}
			MeshSettings.LookupValue("InitialSubdivisionToFinal", initialsubtofinal);

			theMesh = std::unique_ptr<Mesh>(new SquareSubdivisionMesh(maxpixels,
				initialpixels, maxsubdivide,
				iterationpixels, initialsubtofinal, valdiag));
		}
		else if (meshname == "SquareSubdivisionMeshV2")
		{
			largecounter initialpixels{ 100 };
			largecounter maxpixels{ 100 };
			largecounter iterationpixels{ 100 };
			int maxsubdivide{ 1 };
			bool initialsubtofinal{ false };
			MeshSettings.LookupValueInteger("InitialPixels", initialpixels);
			MeshSettings.LookupValueInteger("MaxPixels", maxpixels);
			MeshSettings.LookupValueInteger("IterationPixels", iterationpixels);
			MeshSettings.LookupValue("MaxSubdivide", maxsubdivide);
			if (maxpixels < initialpixels)
				maxpixels = initialpixels;
			if (maxsubdivide < 1) // 1 is the minimum level (initial grid is level 1)
			{
				ScreenOutput("Invalid MaxSubdivide level given. Using MaxSubdivide = 1.", Output_Other_Default);
				maxsubdivide = 1;
			}
			MeshSettings.LookupValue("InitialSubdivisionToFinal", initialsubtofinal);

			theMesh = std::unique_ptr<Mesh>(new SquareSubdivisionMeshV2(maxpixels,
				initialpixels, maxsubdivide,
				iterationpixels, initialsubtofinal, valdiag));
		}
		// else if ... (test for other Meshs here)
		else
		{
			throw SettingError("Incorrect Mesh Type specified.");
		}
	}
	catch (SettingError& e)
	{
		// default Mesh options (SET HERE)
		ScreenOutput(std::string(e.what()) + " Using default (SimpleSquareMesh with 100x100 pixels).",
			Output_Important_Default);
		theMesh = std::unique_ptr<Mesh>(new SimpleSquareMesh{ 100 * 100,valdiag });
	}

	return theMesh;
}



/// <summary>
/// Config::GetGeodesicIntegrator():  Returns a pointer to the integrator function to be used
/// as specified in the configuration file.
/// </summary>
GeodesicIntegratorFunc Config::GetGeodesicIntegrator(const ConfigCollection& theCfg)
{
	std::string IntegratorType{};

	// SET DEFAULTS HERE: RK4 integrator and stepsize=0.03
	GeodesicIntegratorFunc TheFunc = Integrators::IntegrateGeodesicStep_RK4;
	real stepsize{ Integrators::epsilon };
	real hval{ Integrators::Derivative_hval };
	real smalleststep{ Integrators::SmallestPossibleStepsize };
	std::string DefaultString{ "RK4 integrator" };

	try
	{
		// Check to see that there is an integrator set
		if (!theCfg.Exists("Integrator"))
		{
			throw SettingError("No integrator settings found.");
		}
		// Go to the Integrator settings
		const ConfigCollection& IntegratorSettings = theCfg["Integrator"];

		// Look up the integrator step size
		if (!IntegratorSettings.LookupValue("StepSize", stepsize))
		{
			ScreenOutput("Using default integrator stepsize: " + std::to_string(Integrators::epsilon) + ".",
				Output_Other_Default);
		}
		else
		{
			Integrators::epsilon = stepsize;
		}

		// Look up derivative h value (no default message necessary)
		IntegratorSettings.LookupValue("DerivativeH", hval);
		Integrators::Derivative_hval = hval;

		// Look up smallest possible step size (no default message necessary)
		IntegratorSettings.LookupValue("SmallestPossibleStepsize", smalleststep);
		Integrators::SmallestPossibleStepsize = smalleststep;

		// Check to see that the Integrator type has been specified
		if (!IntegratorSettings.LookupValue("Type", IntegratorType))
		{
			throw SettingError("No integrator settings found.");
		}
		// Integrator type exists; now we can look up the correct integrator
		// Make IntegratorType all lower case to avoid case-specific errors
		std::transform(IntegratorType.begin(), IntegratorType.end(), IntegratorType.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		// COMPARISON WITH ALL LOWER CASE LETTERS!
		if (IntegratorType == "rk4")
		{
			// Set the integrator function
			TheFunc = Integrators::IntegrateGeodesicStep_RK4;
			Integrators::IntegratorDescription = "RK4";
		}
		else if (IntegratorType == "verlet")
		{
			// Set the integrator function
			TheFunc = Integrators::IntegrateGeodesicStep_Verlet;
			Integrators::IntegratorDescription = "Verlet";
			
			// Get the velocity tolerance (fractional difference in intermediate and final velocities for a step)
			real verlettolerance{ Integrators::VerletVelocityTolerance };
			IntegratorSettings.LookupValue("VerletVelocityTolerance", verlettolerance);
			Integrators::VerletVelocityTolerance = verlettolerance;
		}
		// else if ... (other integrators here)
		else // no match found: must be incorrect integrator type specified in configuration file
		{
			throw SettingError("No integrator settings found.");
		}
	}
	catch (SettingError& e)
	{
		// Something happened so that we were unable to determine even which integrator to use.
		// Use the default settings (set above at begin of this function)
		ScreenOutput(std::string(e.what()) + " Using default integrator (" + DefaultString + ").",
			Output_Important_Default);
	}

	return TheFunc;
}



/// <summary>
/// Config::GetOutputHandler():  Creates the GeodesicOutputHandler object with options specified
/// according to the configuration file, for handling of geodesic outputs.
/// </summary>
std::unique_ptr<GeodesicOutputHandler> Config::GetOutputHandler(const ConfigCollection& theCfg,
	DiagBitflag alldiags, DiagBitflag valdiag, std::string FirstLineInfo)
{
	// First populate a helper vector of strings of the names of all diagnostics
	std::vector<std::string>diagstrings{ std::move(Utilities::GetDiagNameStrings(alldiags, valdiag)) };

	// DEFAULTS
	std::unique_ptr<GeodesicOutputHandler> TheHandler{ new GeodesicOutputHandler("","","",diagstrings)};

	try
	{
		// Check to see that there are Output settings at all
		if (!theCfg.Exists("Output"))
		{
			throw SettingError("No output handler settings found.");
		}

		// Go to the Output settings
		const ConfigCollection& OutputSettings = theCfg["Output"];


		std::string FilePrefix{};
		// Check to see that the output file name prefix has been specified
		if (!OutputSettings.LookupValue("FilePrefix", FilePrefix))
		{
			throw SettingError("No output file name prefix found.");
		}

		// File extension
		std::string FileExtension{ "" };
		OutputSettings.LookupValue("FileExtension", FileExtension);

		// Print a time stamp in the file name or not
		bool TimeStamp{ true };
		OutputSettings.LookupValue("TimeStamp", TimeStamp);
		std::string TimeStampStr{ "" };
		if (TimeStamp)
		{
			TimeStampStr = std::move(Utilities::GetTimeStampString());
		}

		// Max number of geodesics to cache
		largecounter nrToCache{ LARGECOUNTER_MAX-1 }; // default is essentially infinite
		OutputSettings.LookupValueInteger("GeodesicsToCache", nrToCache);

		// Max number of geodesics to write to file
		largecounter GeodesicsPerFile{ LARGECOUNTER_MAX }; // default is essentially infinite
		OutputSettings.LookupValueInteger("GeodesicsPerFile", GeodesicsPerFile);
		if (GeodesicsPerFile == 0)
			GeodesicsPerFile = LARGECOUNTER_MAX;

		// Write a description line as the first line in every file or not
		bool FirstLineInfoOn{ true };
		OutputSettings.LookupValue("FirstLineInfo", FirstLineInfoOn);

		std::string FirstLineInfoString{ "" };
		if (FirstLineInfoOn)
			FirstLineInfoString = FirstLineInfo;

		// Create the Output Handler!
		TheHandler = std::unique_ptr<GeodesicOutputHandler>(new GeodesicOutputHandler(FilePrefix, TimeStampStr,
															FileExtension,diagstrings, 
															nrToCache,
															GeodesicsPerFile,FirstLineInfoString) );
	}
	catch (SettingError& e)
	{
		// No settings found or no file name found. Will output to console
		ScreenOutput(std::string(e.what()) + " Will do all output to console.",
			Output_Important_Default);
	}

	return TheHandler;
}



#endif // CONFIGURATION_MODE


