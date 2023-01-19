#ifndef _FOORT_CONFIG_H
#define _FOORT_CONFIG_H

///////////////////////////////////////////////////////////////////////////////////////
////// CONFIG.H
////// Functions that read the configuration file (with libconfig)
////// and initialize all objects.
////// All definitions in Config.cpp
///////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////
// COMMENT ONLY THIS LINE OUT TO BE IN PRECOMPILED OPTIONS MODE
#define CONFIGURATION_MODE
//////////////////////////////////////////////////////////


// We need essentially all of the possible different objects as configuration functions initialize all of them
#include "Geometry.h"
#include "Metric.h"
#include "Diagnostics.h"
#include "Terminations.h"
#include "ViewScreen.h"
#include "Geodesic.h"
#include "Integrators.h"


// The entire configuration namespace and its functions are only defined in CONFIGURATION MODE!
#ifdef CONFIGURATION_MODE

#include <memory> // std::unique_ptr
#include <string> // std::string

#include <exception> // needed to define our own configuration error
#include <libconfig.h++> // needed for libconfig functionality


// Namespace for all configuration functions that initialize objects based on configuration file
namespace Config
{
	// Output level for important missing information that will default
	// (e.g. no metric selected, no diagnostics selected, ...)
	constexpr auto Output_Important_Default= OutputLevel::Level_0_WARNING;
	// Output level for less important information that will default
	// (e.g. Kerr metric a parameter not specified)
	constexpr auto Output_Other_Default = OutputLevel::Level_1_PROC;

	// Total configuration object, as loaded from configuration file
	using ConfigObject = libconfig::Config;
	// A setting within the configuration file
	using ConfigSetting = libconfig::Setting;

	// An exception to throw whenever an important setting is not found
	// Always should be caught and then reverted to default settings
	using SettingError = std::invalid_argument;

	// Helper function to look up largecounter options
	bool lookupValuelargecounter(const ConfigSetting& theSetting, const char *name, largecounter &value);


	//// Initialization functions ////
	//////////////////////////////////
	
	

	// Use configuration to initialize the screen output level
	void InitializeScreenOutput(const ConfigObject& theCfg);

	// Use configuration to create the correct Metric with specified parameters
	std::unique_ptr<Metric> GetMetric(const ConfigObject& theCfg);

	// Use configuration to create the correct Source with specified parameters
	std::unique_ptr<Source> GetSource(const ConfigObject& theCfg, const Metric* const theMetric);

	// Use configuration to set the Diagnostics bitflag appropriately;
	// initialize all DiagnosticOptions for all Diagnostics that are turned on;
	// and set bitflag for diagnostic to be used for coarseness evaluating in Mesh
	void InitializeDiagnostics(const ConfigObject& theCfg, DiagBitflag& alldiags, DiagBitflag& valdiag, const Metric* const theMetric);

	// Use configuration to set the Terminations bitflag appropriately;
	// initialize all TerminationOptions for all Terminations that are turned on;
	void InitializeTerminations(const ConfigObject& theCfg, TermBitflag& allterms, const Metric* const theMetric);

	// Use configuration to create ViewScreen appropriately
	std::unique_ptr<ViewScreen> GetViewScreen(const ConfigObject& theCfg, DiagBitflag valdiag, const Metric* const theMetric);
	// GetViewScreen calls GetMesh to create the correct Mesh
	std::unique_ptr<Mesh> GetMesh(const ConfigObject& theCfg, DiagBitflag valdiag);

	// Use configuration to return a pointer to the correct integration function to use
	GeodesicIntegratorFunc GetGeodesicIntegrator(const ConfigObject& theCfg);

	// Use configuration to initialize the output handler
	std::unique_ptr<GeodesicOutputHandler> GetOutputHandler(const ConfigObject& theCfg,
		DiagBitflag alldiags, DiagBitflag valdiag, std::string FirstLineInfo );

} // end namespace Config


#endif // CONFIGURATION_MODE

#endif
