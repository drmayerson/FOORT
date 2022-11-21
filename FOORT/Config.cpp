#include"Config.h"

#include <string>
#include<chrono>
#include<sstream>
#include<iomanip>

/// <summary>
/// SelectMetric switch:  Use configuration to create the correct metric with specified parameters
/// </summary>
std::unique_ptr<Metric> Config::GetMetric(const ConfigObject& theCfg)
{
	std::string MetricName{};

	// SET DEFAULTS HERE: Kerr with a = 0.5, epsHorizon = 0.01
	std::unique_ptr<Metric> TheMetric{ new KerrMetric{0.5} };
	std::string DefaultString{ "Kerr with a = 0.5" };

	// Get the root collection
	ConfigSetting& root = theCfg.getRoot();

	try
	{
		// Check to see that there are Metric settings at all
		if (!root.exists("Metric"))
		{
			throw SettingError("No metric settings found.");
		}

		// Go to the Metric settings
		ConfigSetting& MetricSettings = root["Metric"];

		// Check to see that the Metric's name has been specified
		if (!MetricSettings.lookupValue("Name", MetricName))
		{
			throw SettingError("No metric settings found.");
		}

		// Metric name exists; now we can look up the correct metric
		// Make MetricName all lower case to avoid case-specific errors
		std::transform(MetricName.begin(), MetricName.end(), MetricName.begin(),
			[](unsigned char c) { return std::tolower(c); });

		// COMPARISON WITH ALL LOWER CASE LETTERS!
		if (MetricName == "kerr")
		{
			// Kerr
			
			// First setting to look up: the a parameter
			double theKerra{ 0.5 };
			if (!MetricSettings.lookupValue("a", theKerra))
			{
				ScreenOutput("Kerr: no value for a given. Using default: " + std::to_string(theKerra) + ".",
					Output_Other_Default);
			}

			// Second setting to look up: using a logarithmic r coordinate or not.
			// Don't need to output message if setting not found
			bool rLogScale{ false };
			MetricSettings.lookupValue("RLogScale", rLogScale);

			// All settings complete; create Metric object!
			TheMetric = std::unique_ptr<Metric>(new KerrMetric{ theKerra, rLogScale });
		}
		else if (MetricName == "flatspace")
		{
			// (4D) flat space in spherical coordinates; no options necessary
			// Create Metric object!
			TheMetric = std::unique_ptr<Metric>(new FlatSpaceMetric{});
		}
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


// DECLARATION OF ALL static DiagnosticOptions (for all types of Diagnostics) needed here!
std::unique_ptr<DiagnosticOptions> FourColorScreenDiagnostic::DiagOptions;
std::unique_ptr<GeodesicPositionOptions> GeodesicPositionDiagnostic::DiagOptions;
std::unique_ptr<DiagnosticOptions> EquatorialPassesDiagnostic::DiagOptions;



/// <summary>
/// Diagnostics switch:  Use configuration to set the Diagnostics bitflag appropriately;
/// initialize all DiagnosticOptions for all Diagnostics that are turned on;
/// and set bitflag for diagnostic to be used for coarseness evaluating in Mesh
/// </summary>
void Config::InitializeDiagnostics(const ConfigObject& theCfg, DiagBitflag& alldiags, DiagBitflag& valdiag)
{
	// First set these flags to all zeros
	alldiags = Diag_None;
	valdiag = Diag_None;

	// Get the root configuration settings
	ConfigSetting& root = theCfg.getRoot();

	try
	{
		// Check to see that there are Diagnostics settings at all
		if (!root.exists("Diagnostics"))
		{
			throw SettingError("No diagnostic settings found.");
		}

		// Go to the Diagnostics settings
		ConfigSetting& AllDiagSettings = root["Diagnostics"];

		// Helper function: checks to see if a particular Diagnostic (with name Diagname)
		// is present in the Diagnostics options; if the setting "on" is specified;
		// and if "on" is also set to "true" (so by default "on" = false)
		// Only if all these are satisfied do we want to have the Diagnostic included (and initialized)
		auto CheckIfDiagOn = [&AllDiagSettings](const std::string& DiagName)
		{
			bool diagison{ false };
			return AllDiagSettings.exists(DiagName.c_str()) // note that g++ on Fabio's Linux requires the .c_str() everywhere
				&& AllDiagSettings[DiagName.c_str()].exists("On")
				&& AllDiagSettings[DiagName.c_str()].lookupValue("On", diagison)
				&& diagison;
		};

		// FourColorScreen
		if (CheckIfDiagOn("FourColorScreen"))
		{
			// Four color screen diagnostic on! Add it to bitflag.
			alldiags |= Diag_FourColorScreen;

			// By default, this Diagnostic only updates at the end of each geodesic.
			// Check to see if a different update frequency has been specified
			int updatefreq = Update_OnlyFinish;
			AllDiagSettings["FourColorScreen"].lookupValue("UpdateFrequency", updatefreq);
			// Initialize the (static) DiagnosticOptions for FourColorScreen!
			FourColorScreenDiagnostic::DiagOptions = std::unique_ptr<DiagnosticOptions>(new DiagnosticOptions{ updatefreq });

			// check to see if there is no valdiag set yet;
			// and there is a option "UseForMesh" specied for this Diagnostic;
			// and "UseForMesh" is set to true
			bool isVal{ false };
			if (valdiag == Diag_None && AllDiagSettings["FourColorScreen"].lookupValue("UseForMesh", isVal) && isVal)
			{
				// Use this Diagnostic for the Mesh values
				valdiag = Diag_FourColorScreen;
			}
		}
		// GeodesicPosition
		if (CheckIfDiagOn("GeodesicPosition"))
		{
			alldiags |= Diag_GeodesicPosition;

			int updatefreq = 1; // update every step
			AllDiagSettings["GeodesicPosition"].lookupValue("UpdateFrequency", updatefreq);

			int outputsteps = -1; // keep all steps
			AllDiagSettings["GeodesicPosition"].lookupValue("OutputSteps", outputsteps);

			GeodesicPositionDiagnostic::DiagOptions = 
				std::unique_ptr<GeodesicPositionOptions>(new GeodesicPositionOptions{ outputsteps, updatefreq });

			bool isVal{ false };
			if (valdiag == Diag_None && AllDiagSettings["GeodesicPosition"].lookupValue("UseForMesh", isVal) && isVal)
			{
				// Use this Diagnostic for the Mesh values
				valdiag = Diag_GeodesicPosition;
			}
		}
		// GeodesicPosition
		if (CheckIfDiagOn("EquatorialPasses"))
		{
			alldiags |= Diag_EquatorialPasses;

			int updatefreq = 1; // update every step
			AllDiagSettings["EquatorialPasses"].lookupValue("UpdateFrequency", updatefreq);

			EquatorialPassesDiagnostic::DiagOptions =
				std::unique_ptr<DiagnosticOptions>(new DiagnosticOptions{ updatefreq });

			bool isVal{ false };
			if (valdiag == Diag_None && AllDiagSettings["EquatorialPasses"].lookupValue("UseForMesh", isVal) && isVal)
			{
				// Use this Diagnostic for the Mesh values
				valdiag = Diag_EquatorialPasses;
			}
		}
		// if (CheckIfDiagOn("..."))
		// ...
		// ... (other ifs for other diagnostics here)



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
		FourColorScreenDiagnostic::DiagOptions = std::unique_ptr<DiagnosticOptions>(new DiagnosticOptions{ Update_OnlyFinish });
	}

}

// DECLARATION OF ALL static TerminationOptions (for all types of Terminations) needed here!
std::unique_ptr<HorizonTermOptions> HorizonTermination::TermOptions;
std::unique_ptr<BoundarySphereTermOptions> BoundarySphereTermination::TermOptions;
std::unique_ptr<TimeOutTermOptions> TimeOutTermination::TermOptions;


/// <summary>
/// Terminations switch:  Use configuration to set the Termination bitflag appropriately;
/// initialize all TerminationOptions for all Terminations that are turned on;
/// </summary>
void Config::InitializeTerminations(const ConfigObject& theCfg, TermBitflag& allterms, const Metric* theMetric)
{
	// First set the flag to all zeros
	allterms = Term_None;

	// Get the root configuration settings
	ConfigSetting& root = theCfg.getRoot();

	try
	{
		// Check to see that there are Terminations settings at all
		if (!root.exists("Terminations"))
		{
			throw SettingError("No termination settings found.");
		}

		// Go to the Terminations settings
		ConfigSetting& AllTermSettings = root["Terminations"];

		// Helper function: checks to see if a particular Termination (with name TermName)
		// is present in the Terminations options; if the setting "on" is specified;
		// and if "on" is also set to "true" (so by default "on" = false)
		// Only if all these are satisfied do we want to have the Termination included (and initialized)
		auto CheckIfTermOn = [&AllTermSettings](const std::string& TermName)
		{
			bool termison{ false };
			return AllTermSettings.exists(TermName.c_str()) // note that g++ on Fabio's Linux requires the .c_str() everywhere
				&& AllTermSettings[TermName.c_str()].exists("On")
				&& AllTermSettings[TermName.c_str()].lookupValue("On", termison)
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
				AllTermSettings["Horizon"].lookupValue("Epsilon_Horizon", epsHorizon);

				// By default, this Termination updates every step
				// Check to see if a different update frequency has been specified
				int updatefreq = 1;
				AllTermSettings["Horizon"].lookupValue("UpdateFrequency", updatefreq);

				// Initialize the (static) TerminationOptions for Horizon!
				HorizonTermination::TermOptions =
					std::unique_ptr<HorizonTermOptions>(new HorizonTermOptions{ horizonRadius,rLogScale,epsHorizon,updatefreq });
			}
		}

		// BoundarySphere
		if (CheckIfTermOn("BoundarySphere"))
		{
			// Boundary sphere termination on! Add it to bitflag.
			allterms |= Term_BoundarySphere;

			// Get the radius of the boundary sphere. Default is 1000(M).
			real radius{ 1000 };
			AllTermSettings["BoundarySphere"].lookupValue("SphereRadius", radius);


			// By default, this Termination updates every step
			// Check to see if a different update frequency has been specified
			int updatefreq = 1;
			AllTermSettings["BoundarySphere"].lookupValue("UpdateFrequency", updatefreq);

			// Initialize the (static) TerminationOptions for BoundarySphere!
			BoundarySphereTermination::TermOptions =
				std::unique_ptr<BoundarySphereTermOptions>(new BoundarySphereTermOptions{ radius, updatefreq });
		}

		// TimeOut
		if (CheckIfTermOn("TimeOut"))
		{
			// Time out termination on! Add it to bitflag.
			allterms |= Term_TimeOut;

			// Get the number of steps until time-out. Default is 10000
			int timeoutsteps {10000 };
			AllTermSettings["TimeOut"].lookupValue("MaxSteps", timeoutsteps);


			// By default, this Termination updates every step
			// Check to see if a different update frequency has been specified
			int updatefreq = 1;
			AllTermSettings["TimeOut"].lookupValue("UpdateFrequency", updatefreq);

			// Initialize the (static) TerminationOptions for TimeOut!
			TimeOutTermination::TermOptions =
				std::unique_ptr<TimeOutTermOptions>(new TimeOutTermOptions{ timeoutsteps, updatefreq });
		}
		// if (CheckIfTermOn("..."))
		// ...
		// ... (other ifs for other Terminations here)


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
		ScreenOutput(std::string(e.what()) + " Using default termination(s) (" + "BoundarySphere \& TimeOut" + ").",
			Output_Important_Default);
		allterms= Term_BoundarySphere | Term_TimeOut;
		BoundarySphereTermination::TermOptions =
			std::unique_ptr<BoundarySphereTermOptions>(new BoundarySphereTermOptions{ 1000, 1 });
		TimeOutTermination::TermOptions =
			std::unique_ptr<TimeOutTermOptions>(new TimeOutTermOptions{ 10000, 1 });
	}

}

std::unique_ptr<Mesh> Config::GetMesh(const ConfigObject& theCfg, DiagBitflag valdiag)
{
	// Get the root configuration settings
	ConfigSetting& root = theCfg.getRoot();

	std::unique_ptr<Mesh> theMesh;

	try
	{
		if (!root.exists("ViewScreen") || !root["ViewScreen"].exists("Mesh"))
		{
			throw SettingError("No Mesh settings found.");
		}

		ConfigSetting& MeshSettings = root["ViewScreen"]["Mesh"];

		std::string meshname{};
		if (!MeshSettings.lookupValue("Type", meshname))
		{
			throw SettingError("No Mesh Type specified.");
		}

		if (meshname == "SimpleSquareMesh")
		{
			// Simple Square Mesh!
			int totalpixels{ 100 * 100 };
			MeshSettings.lookupValue("TotalPixels", totalpixels);

			theMesh = std::unique_ptr<Mesh>(new SimpleSquareMesh(totalpixels,valdiag));
		}
		else if (meshname == "InputCertainPixelsMesh")
		{
			int totalpixels{ 100 * 100 };
			MeshSettings.lookupValue("TotalPixels", totalpixels);

			theMesh = std::unique_ptr<Mesh>(new InputCertainPixelsMesh(totalpixels,valdiag));
		}
		else if (meshname == "SquareSubdivisionMesh")
		{
			int initialpixels{ 100 };
			int maxpixels{ 100 };
			int iterationpixels{ 100 };
			int maxsubdivide{ 1 };
			bool initialsubtofinal{ false };
			MeshSettings.lookupValue("InitialPixels", initialpixels);
			MeshSettings.lookupValue("MaxPixels", maxpixels);
			MeshSettings.lookupValue("IterationPixels", iterationpixels);
			MeshSettings.lookupValue("MaxSubdivide", maxsubdivide);
			MeshSettings.lookupValue("InitialSubdivisionToFinal", initialsubtofinal);

			bool infinitepixels{ maxpixels < 0 };

			theMesh = std::unique_ptr<Mesh>(new SquareSubdivisionMesh(maxpixels,initialpixels,maxsubdivide,
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


std::unique_ptr<ViewScreen> Config::GetViewScreen(const ConfigObject& theCfg, DiagBitflag valdiag, const Metric* theMetric)
{
	ConfigSetting& root = theCfg.getRoot();

	// DEFAULTS set here
	Point pos{ 0,1000,pi/2.0,0 };
	OneIndex dir{ 0,-1,0,0 };
	std::array<real, dimension - 2> screensize{ pi*1/10,pi*1/10 };

	try
	{
		// Check to see that there are ViewScreen settings at all
		if (!root.exists("ViewScreen"))
		{
			throw SettingError("No view screen settings found.");
		}

		// Go to the ViewScreen settings
		ConfigSetting& ViewSettings = root["ViewScreen"];

		// Look up camera position, direction, solidangle
		if (ViewSettings.exists("Position"))
		{
			ViewSettings["Position"].lookupValue("t", pos[0]);
			ViewSettings["Position"].lookupValue("r", pos[1]);
			ViewSettings["Position"].lookupValue("theta", pos[2]);
			ViewSettings["Position"].lookupValue("phi", pos[3]);
		}
		if (ViewSettings.exists("Direction"))
		{
			ViewSettings["Direction"].lookupValue("t", dir[0]);
			ViewSettings["Direction"].lookupValue("r", dir[1]);
			ViewSettings["Direction"].lookupValue("theta", dir[2]);
			ViewSettings["Direction"].lookupValue("phi", dir[3]);
		}
		if (ViewSettings.exists("ScreenSize"))
		{
			ViewSettings["ScreenSize"].lookupValue("x", screensize[0]);
			ViewSettings["ScreenSize"].lookupValue("y", screensize[1]);
		}
	}
	catch (SettingError& e)
	{
		ScreenOutput(std::string(e.what()) + " Using default ViewScreen Settings.",
			Output_Important_Default);
	}

	std::unique_ptr<Mesh> theMesh{ Config::GetMesh(theCfg,valdiag)};

	std::unique_ptr<ViewScreen> theViewScreen{ new ViewScreen(pos, dir, screensize,
		std::move(theMesh),theMetric) };

	return theViewScreen;
}


std::unique_ptr<Source> Config::GetSource(const ConfigObject& theCfg, const Metric* theMetric)
{
	std::string SourceName{};

	// SET DEFAULTS HERE: no source
	std::unique_ptr<Source> TheSource{ new NoSource(theMetric) };
	std::string DefaultString{ "No source." };

	// Get the root collection
	ConfigSetting& root = theCfg.getRoot();

	try
	{
		// Check to see that there are Source settings at all
		if (!root.exists("Source"))
		{
			throw SettingError("No geodesic source settings found.");
		}

		// Go to the Source settings
		ConfigSetting& SourceSettings = root["Source"];

		// Check to see that the Source's name has been specified
		if (!SourceSettings.lookupValue("Name", SourceName))
		{
			throw SettingError("No source settings found.");
		}

		// Source name exists; now we can look up the correct source
		// Make SourceName all lower case to avoid case-specific errors
		std::transform(SourceName.begin(), SourceName.end(), SourceName.begin(),
			[](unsigned char c) { return std::tolower(c); });

		// COMPARISON WITH ALL LOWER CASE LETTERS!
		if (SourceName == "nosource")
		{
			// No further settings needed; create Source!
			TheSource = std::unique_ptr<Source>(new NoSource( theMetric ));
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

GeodesicIntegratorFunc Config::GetGeodesicIntegrator(const ConfigObject& theCfg)
{
	std::string IntegratorType{};

	// SET DEFAULTS HERE: RK4 integrator and stepsize=0.03
	GeodesicIntegratorFunc TheFunc = Integrators::IntegrateGeodesicStep_RK4;
	real stepsize{ Integrators::epsilon };
	std::string DefaultString{ "RK4 integrator" };

	// Get the root collection
	ConfigSetting& root = theCfg.getRoot();

	try
	{
		// Check to see that there is an integrator set
		if (!root.exists("Integrator"))
		{
			throw SettingError("No integrator settings found.");
		}
		// Go to the Integrator settings
		ConfigSetting& IntegratorSettings = root["Integrator"];

		// Check to see that the Integrator type has been specified
		if (!IntegratorSettings.lookupValue("Type", IntegratorType))
		{
			throw SettingError("No integrator settings found.");
		}
		// Integrator type exists; now we can look up the correct integrator
		// Make IntegratorType all lower case to avoid case-specific errors
		std::transform(IntegratorType.begin(), IntegratorType.end(), IntegratorType.begin(),
			[](unsigned char c) { return std::tolower(c); });

		// COMPARISON WITH ALL LOWER CASE LETTERS!
		if (IntegratorType == "rk4")
		{
			// Set the integrator function
			TheFunc = Integrators::IntegrateGeodesicStep_RK4;

			if (!IntegratorSettings.lookupValue("StepSize", stepsize))
			{
				ScreenOutput("Using default integrator stepsize: " + std::to_string(Integrators::epsilon) + ".", Output_Other_Default);
			}
			else
			{
				Integrators::epsilon = stepsize;
			}
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

void Config::InitializeScreenOutput(const ConfigObject& theCfg)
{
	SetOutputLevel(OutputLevel::Level_4_DEBUG);
	OutputLevel theOutputLevel{};
	// Get the root collection
	ConfigSetting& root = theCfg.getRoot();
	if (root.exists("Output"))
	{
		ConfigSetting& OutputSettings = root["Output"];
		int scroutputint{ static_cast<int>(OutputLevel::Level_4_DEBUG) };
		OutputSettings.lookupValue("ScreenOutputLevel", scroutputint);
		scroutputint = std::max(scroutputint, static_cast<int>(OutputLevel::Level_0_WARNING));
		scroutputint = std::min(scroutputint, static_cast<int>(OutputLevel::MaxLevel));
		SetOutputLevel(static_cast<OutputLevel>(scroutputint));
	}
}

std::unique_ptr<GeodesicOutputHandler> Config::InitializeOutputHandler(const ConfigObject& theCfg, 
	DiagBitflag alldiags, DiagBitflag valdiag, std::string FirstLineInfo)
{
	std::vector<std::string>diagstrings{};
	// Construct vector of Diagnostic names in temporary scope
	{
		DiagnosticUniqueVector tempDiags{ CreateDiagnosticVector(alldiags, valdiag, nullptr) };
		diagstrings.reserve(tempDiags.size());
		for (const auto& d : tempDiags)
			diagstrings.push_back(d->GetDiagNameStr());
	}

	// DEFAULTS
	std::unique_ptr<GeodesicOutputHandler> TheHandler{ new GeodesicOutputHandler("","","",diagstrings,OutputHandler_All)};

	// Get the root collection
	ConfigSetting& root = theCfg.getRoot();

	try
	{
		// Check to see that there are Output settings at all
		if (!root.exists("Output"))
		{
			throw SettingError("No output handler settings found.");
		}

		// Go to the Output settings
		ConfigSetting& OutputSettings = root["Output"];


		std::string FilePrefix{};
		// Check to see that the output file name prefix has been specified
		if (!OutputSettings.lookupValue("FilePrefix", FilePrefix))
		{
			throw SettingError("No output file name prefix found.");
		}

		std::string FileExtension{ "" };
		OutputSettings.lookupValue("FileExtension", FileExtension);

		bool TimeStamp{ true };
		OutputSettings.lookupValue("TimeStamp", TimeStamp);
		std::string TimeStampStr{ "" };
		if (TimeStamp)
		{
			std::time_t t = std::time(nullptr);
			std::tm tm = *std::localtime(&t);
			std::stringstream datetime;
			datetime << std::put_time(&tm, "%y%m%d-%H%M%S");
			TimeStampStr = datetime.str();
			//TimeStampStr = "TIMESTAMP";
		}

		int nrToCache{ OutputHandler_All };
		OutputSettings.lookupValue("GeodesicToCache", nrToCache);

		int GeodesicsPerFile{ OutputHandler_All };
		OutputSettings.lookupValue("GeodesicsPerFile", GeodesicsPerFile);

		bool FirstLineInfoOn{ true };
		OutputSettings.lookupValue("FirstLineInfo", FirstLineInfoOn);

		std::string FirstLineInfoString{ "" };
		if (FirstLineInfoOn)
			FirstLineInfoString = FirstLineInfo;

		TheHandler = std::unique_ptr<GeodesicOutputHandler>(new GeodesicOutputHandler(FilePrefix, TimeStampStr,
															FileExtension,diagstrings, nrToCache, GeodesicsPerFile,FirstLineInfoString) );
	}
	catch (SettingError& e)
	{
		// No settings found or no file name found. Will output to console
		ScreenOutput(std::string(e.what()) + " Will do all output to console.",
			Output_Important_Default);
	}

	return TheHandler;
}

std::string Config::GetFirstLineInfoString(const Metric* theMetric, const Source* theSource, DiagBitflag alldiags, DiagBitflag valdiag,
	TermBitflag allterms, const ViewScreen* theView, GeodesicIntegratorFunc theIntegrator)
{
	/*std::string fulldiagstring{"Diagnostics: "};
	{ // temp scope to create/destroy this diagnostic vector
		DiagnosticUniqueVector tempdiagvec{ CreateDiagnosticVector(alldiags, valdiag, nullptr) };
		for (auto& d : tempdiagvec)
		{
			fulldiagstring+= d->GetDescriptionString() + ", ";
		}
	}*/

	std::string fulltermstring{ "Terminations: " };
	{ // temp scope to create/destroy this termination vector
		TerminationUniqueVector temptermvec{ CreateTerminationVector(allterms, nullptr) };
		for (auto& t : temptermvec)
		{
			fulltermstring += t->GetDescriptionString() + ", ";
		}
	}


	return "Metric: " + theMetric->GetDescriptionString() + "; "
		+ "Source: " + theSource->GetDescriptionString() + "; "
		+ fulltermstring + "; " + theView->GetDescriptionstring();
}


