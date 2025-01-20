#include "Utilities.h" // We are implementing functions from here

// These are used in GetTimeStampString()
#include <sstream>
#include <iomanip>

/**
 * @file Utilities.cpp
 * @author Daniel R. Mayerson
 * @version 0.1
 * @date 2025-01-14
 * @copyright Copyright (c) 2025
 */

// Utilities::Timer functions

/**
 * @brief Reset begin time
 */
void Utilities::Timer::reset()
{
	m_beg = Clock::now();
}

/**
 * @brief Returns time elapsed since begin time
 * @return double
 */
double Utilities::Timer::elapsed() const
{
	return std::chrono::duration_cast<Second>(Clock::now() - m_beg).count();
}

// Other functions in Utilities

/**
 * @brief Get a string of the current time (in a format that can be used to append to file names)
 * @return std::string
 */
std::string Utilities::GetTimeStampString()
{
	std::time_t t = std::time(nullptr);
	std::tm tm = *std::localtime(&t); // using localtime can return a compiler warning stating it is not thread safe!
	std::stringstream datetime;
	// This is a timestamp that will be used to append to file names
	datetime << std::put_time(&tm, "%y%m%d-%H%M%S");
	return datetime.str();
}

/**
 * @brief Helper function to get all Diagnostic Names (for outputting to files)
 * @param alldiags bitflag for all Diagnostics
 * @param valdiag bitflag for the value diagnostic
 * @return std::vector<std::string>
 */
std::vector<std::string> Utilities::GetDiagNameStrings(DiagBitflag alldiags, DiagBitflag valdiag)
{
	std::vector<std::string> thediagstrings{};
	// We temporarily create a vector of all the Diagnostics that are turned on;
	// for the owner Geodesic pointer we simply pass nullptr
	DiagnosticUniqueVector tempDiags{CreateDiagnosticVector(alldiags, valdiag, nullptr)};
	// Then we fill up the vector of names
	thediagstrings.reserve(tempDiags.size());
	for (const auto &d : tempDiags)
		thediagstrings.push_back(std::move(d->getNameStr()));

	return thediagstrings;
}

/**
 * @brief This returns the full string to be written to every output file as its first line
 * @param theMetric pointer to the Metric object
 * @param theSource pointer to the Source object
 * @param alldiags bitflag for all Diagnostics
 * @param valdiag bitflag for the value diagnostic
 * @param allterms bitflag for all Terminations
 * @param theView pointer to the ViewScreen object
 * @return std::string
 */
std::string Utilities::GetFirstLineInfoString(const Metric *theMetric, const Source *theSource,
											  DiagBitflag alldiags, DiagBitflag valdiag,
											  TermBitflag allterms, const ViewScreen *theView)
{
	// Create a string for all Diagnostics information
	std::string fulldiagstring{"Diagnostics: "};
	{ // temp scope to create/destroy this diagnostic vector
		// We simply pass nullptr as the owner Geodesic pointer
		DiagnosticUniqueVector tempdiagvec{CreateDiagnosticVector(alldiags, valdiag, nullptr)};
		for (auto &d : tempdiagvec)
		{
			fulldiagstring += d->getFullDescriptionStr() + ", ";
		}
	}

	// Create a string for all Termination information
	std::string fulltermstring{"Terminations: "};
	{ // temp scope to create/destroy this termination vector
		// We simply pass nullptr as the owner Geodesic pointer
		TerminationUniqueVector temptermvec{CreateTerminationVector(allterms, nullptr)};
		for (auto &t : temptermvec)
		{
			fulltermstring += t->getFullDescriptionStr() + ", ";
		}
	}

	// Full string contains information about the Metric, Source, Diagnostics, Terminations, ViewScreen, Integrators
	return "Metric: " + theMetric->getFullDescriptionStr() + "; " + "Source: " + theSource->getFullDescriptionStr() + "; " + fulldiagstring + "; " + fulltermstring + "; " + theView->getFullDescriptionStr() + "; " + Integrators::GetFullIntegratorDescription();
}
