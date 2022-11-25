#include "Utilities.h"
#include "Diagnostics.h"

#include<string>
#include<sstream>
#include<iomanip>

void Utilities::Timer::reset()
{
    m_beg = Clock::now();
}

double Utilities::Timer::elapsed() const
{
    return std::chrono::duration_cast<Second>(Clock::now() - m_beg).count();
}

std::string Utilities::GetTimeStampString()
{
	std::time_t t = std::time(nullptr);
	std::tm tm = *std::localtime(&t);
	std::stringstream datetime;
	datetime << std::put_time(&tm, "%y%m%d-%H%M%S");
	return datetime.str();
}

std::vector<std::string> Utilities::GetDiagStrings(DiagBitflag alldiags, DiagBitflag valdiag)
{
	std::vector<std::string> thediagstrings{};
	DiagnosticUniqueVector tempDiags{ CreateDiagnosticVector(alldiags, valdiag, nullptr) };
	thediagstrings.reserve(tempDiags.size());
	for (const auto& d : tempDiags)
		thediagstrings.push_back(d->GetDiagNameStr());

	return thediagstrings;
}

std::string Utilities::GetFirstLineInfoString(const Metric* theMetric, const Source* theSource, DiagBitflag alldiags, DiagBitflag valdiag,
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