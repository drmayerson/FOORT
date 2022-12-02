#include "Utilities.h" // We are implementing functions from here

// These are used in GetTimeStampString()
#include <sstream>
#include <iomanip>


/// <summary>
/// Utilities::Timer functions
/// </summary>

void Utilities::Timer::reset()
{
	// Reset begin time
    m_beg = Clock::now();
}

double Utilities::Timer::elapsed() const
{
	// return time elapsed since begin time
    return std::chrono::duration_cast<Second>(Clock::now() - m_beg).count();
}

/// <summary>
/// Other functions in Utilities
/// </summary>

std::string Utilities::GetTimeStampString()
{
	std::time_t t = std::time(nullptr);
	std::tm tm = *std::localtime(&t); // using localtime can return a compiler warning stating it is not thread safe!
	std::stringstream datetime;
	// This is a timestamp that will be used to append to file names
	datetime << std::put_time(&tm, "%y%m%d-%H%M%S");
	return datetime.str();
}

std::vector<std::string> Utilities::GetDiagNameStrings(DiagBitflag alldiags, DiagBitflag valdiag)
{
	std::vector<std::string> thediagstrings{};
	// We temporarily create a vector of all the Diagnostics that are turned on;
	// for the owner Geodesic pointer we simply pass nullptr
	DiagnosticUniqueVector tempDiags{ CreateDiagnosticVector(alldiags, valdiag, nullptr) };
	// Then we fill up the vector of names
	thediagstrings.reserve(tempDiags.size());
	for (const auto& d : tempDiags)
		thediagstrings.push_back(d->getNameStr());

	return thediagstrings;
}

std::string Utilities::GetFirstLineInfoString(const Metric* theMetric, const Source* theSource,
	[[maybe_unused]] DiagBitflag alldiags,
	[[maybe_unused]] DiagBitflag valdiag,
	TermBitflag allterms, const ViewScreen* theView,
	[[maybe_unused]] GeodesicIntegratorFunc theIntegrator)
{
	// This returns a descriptive string that is outputted on the first line of every file
	
	// We do not give Diagnostic information: every file is already dedicated to a Diagnostic
	// and has the Diagnostic name in its file name

	// Create a string for all Termination information
	std::string fulltermstring{ "Terminations: " };
	{ // temp scope to create/destroy this termination vector
		// We simply pass nullptr as the owner Geodesic pointer
		TerminationUniqueVector temptermvec{ CreateTerminationVector(allterms, nullptr) };
		for (auto& t : temptermvec)
		{
			fulltermstring += t->getFullDescriptionStr() + ", ";
		}
	}

	// Full string contains information about the Metric, Source, Terminations, and ViewScreen
	return "Metric: " + theMetric->getFullDescriptionStr() + "; "
		+ "Source: " + theSource->getFullDescriptionStr() + "; "
		+ fulltermstring + "; " + theView->getFullDescriptionStr();
}