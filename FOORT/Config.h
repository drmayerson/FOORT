#ifndef _FOORT_CONFIG_H
#define _FOORT_CONFIG_H

#include<exception>
#include<libconfig.h++>

#include"Geometry.h"
#include"Metric.h"
#include"Diagnostics.h"
#include"Terminations.h"
#include"ViewScreen.h"
#include"Geodesic.h"
#include"Integrators.h"

namespace Config
{
	// Output level for important missing information that will default
	// (e.g. no metric selected, no diagnostics selected, ...)
	constexpr auto Output_Important_Default= OutputLevel::Level_0_NONE;
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

	// Use configuration to create the correct metric with specified parameters
	std::unique_ptr<Metric> GetMetric(const ConfigObject& theCfg);

	// Use configuration to set the Diagnostics bitflag appropriately;
	// initialize all DiagnosticOptions for all Diagnostics that are turned on;
	// and set bitflag for diagnostic to be used for coarseness evaluating in Mesh
	void InitializeDiagnostics(const ConfigObject& theCfg, DiagBitflag& alldiags, DiagBitflag& valdiag);

	// Use configuration to set the Terminations bitflag appropriately;
	// initialize all TerminationOptions for all Terminations that are turned on;
	void InitializeTerminations(const ConfigObject& theCfg, TermBitflag& allterms);

	std::unique_ptr<Mesh> GetMesh(const ConfigObject& theCfg, DiagBitflag valdiag);
	std::unique_ptr<ViewScreen> GetViewScreen(const ConfigObject& theCfg, DiagBitflag valdiag, const Metric* theMetric);

	std::unique_ptr<Source> GetSource(const ConfigObject& theCfg, const Metric* theMetric);

	GeodesicIntegratorFunc GetGeodesicIntegrator(const ConfigObject& theCfg);

	std::unique_ptr<GeodesicOutputHandler> GetAndInitializeOutput(const ConfigObject& theCfg);
}





#endif
