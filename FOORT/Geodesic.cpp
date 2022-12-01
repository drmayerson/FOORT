#include"Geodesic.h" // We are implementing Source & Geodesic member functions declared here

#include "InputOutput.h" // for ScreenOutput()

/// <summary>
/// Source (and descendant classes) functions
/// </summary>

std::string Source::getFullDescriptionStr() const
{
	// Basic full description string
	return "Source (no override description specified)";
}

OneIndex NoSource::getSource([[maybe_unused]] Point pos, [[maybe_unused]] OneIndex vel) const
{
	// no rhs for the geodesic equation: no force felt by geodesic
	return OneIndex{ 0,0,0,0 };
}

std::string NoSource::getFullDescriptionStr() const
{
	// Full description string
	return "No source";
}



/// <summary>
/// Geodesic (and descendant classes) functions
/// </summary>


Term Geodesic::Update()
{
	// Integrate one step!
	Point newpos{};
	OneIndex newvel{};
	real step{};
	// The integrator function will set the new position, new velocity, and the (affine parameter) step taken
	m_theIntegrator(m_CurrentPos, m_CurrentVel, newpos, newvel, step, m_theMetric, m_theSource);
	m_curLambda += step;
	m_CurrentPos = newpos;
	m_CurrentVel = newvel;

	// Check all possible termination conditions
	for (const auto& t : m_AllTerminations)
	{
		m_TermCond = t->CheckTermination();
		if (m_TermCond != Term::Continue)
			break;
	}

	// No matter if we terminate now or not, loop through all Diagnostics to update them
	for (const auto& d : m_AllDiagnostics)
	{
		d->UpdateData();
	}

	return m_TermCond;
}

Term Geodesic::getTermCondition() const
{
	return m_TermCond;
}

Point Geodesic::getCurrentPos() const
{
	return m_CurrentPos;
}

OneIndex Geodesic::getCurrentVel() const
{
	return m_CurrentVel;
}

real Geodesic::getCurrentLambda() const
{
	return m_curLambda;
}


std::vector<std::string> Geodesic::getAllOutputStr() const
{
	// The Geodesic should have terminated if this is called!
	if (m_TermCond == Term::Continue)
		ScreenOutput("Geodesic not terminated yet but getAllOutputStr() is called!", OutputLevel::Level_0_WARNING);


	// This gets the complete output that should be written to the output file
	// Every Diagnostic returns a string, PLUS the FIRST string is the screen index
	std::vector<std::string> theOutput{};
	theOutput.reserve(m_AllDiagnostics.size() + 1);

	// First string is the screen index of the Geodesic
	std::string strScreenIndex{ "" };
	for (int i = 0; i < m_ScreenIndex.size(); ++i)
	{
		// We don't use the toString() function to avoid extraneous parentheses and commas therein
		strScreenIndex += std::to_string(m_ScreenIndex[i]) + " ";
	}
	theOutput.push_back(strScreenIndex);

	// The rest of the strings are the output strings as given by each of the Diagnostics
	for (const auto& d : m_AllDiagnostics)
	{
		theOutput.push_back(d->getFullDataStr());
	}

	return theOutput;
}

std::vector<real> Geodesic::getDiagnosticFinalValue() const
{
	// The Geodesic should have terminated if this is called!
	if (m_TermCond == Term::Continue)
		ScreenOutput("Geodesic not terminated yet but getDiagnosticFinalValue() is called!", OutputLevel::Level_0_WARNING);

	// The diagnostic that contributes the value is always at the first position in the Diagnostic array!
	return m_AllDiagnostics[0]->getFinalDataVal();
}