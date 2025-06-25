#include "Geodesic.h" // We are implementing Source & Geodesic member functions declared here

#include "InputOutput.h" // for ScreenOutput()

/**
 * @file Geodesic.cpp
 * @author Daniel R. Mayerson
 * @version 1.0
 * @date 2024-12-16
 * @copyright Copyright (c) 2024
 */

// Source (and descendant classes) functions

/**
 * @brief Basic full description string getter for Source base class
 *
 * @return std::string
 */
std::string Source::getFullDescriptionStr() const
{
	return "Source (no override description specified)";
}

/**
 * @brief Returns zero source for the geodesic equation: no force felt by geodesic
 *
 * @param pos Position of the geodesic
 * @param vel Velocity of the geodesic
 * @return OneIndex Source for the geodesic
 */
OneIndex NoSource::getSource([[maybe_unused]] Point pos, [[maybe_unused]] OneIndex vel) const
{
	return OneIndex{0, 0, 0, 0};
}

/**
 * @brief Full description string getter for NoSource class
 *
 * @return std::string
 */
std::string NoSource::getFullDescriptionStr() const
{
	return "No source";
}

/// <summary>
/// Geodesic (and descendant classes) functions
/// </summary>

/**
 * @brief This initializes/resets the geodesic with a given ScreenIndex, initial position, and initial velocity
 * @details Also resets all Diagnostics and Terminations, resets the TermCondition to Term::Continue,
 *	and puts the Geodesic back to lambda = 0.0.
 *
 * @param scrindex ScreenIndex
 * @param initpos Initial position
 * @param initvel Initial velocity
 */
void Geodesic::Reset(ScreenIndex scrindex, Point initpos, OneIndex initvel)
{
	// Set screen index, initial position/velocity
	m_ScreenIndex = scrindex;
	m_CurrentPos = initpos;
	m_CurrentVel = initvel;

	// Start at 0.0 affine parameter
	m_curLambda = 0.0;

	// Geodesic is set up to be integrated
	m_TermCond = Term::Continue;

	// Start: loop through all diagnostics to reset and update at starting position
	for (const auto &d : m_AllDiagnostics)
	{
		d->Reset();
		d->UpdateData();
	}

	// Also reset all terminations
	for (const auto &t : m_AllTerminations)
	{
		t->Reset();
	}
}

/**
 * @brief This makes the Geodesic integrate itself one step; then the Geodesic loops through all Terminations and Diagnostics to update
 *
 * @return Term
 */
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
	for (const auto &t : m_AllTerminations)
	{
		m_TermCond = t->CheckTermination();
		if (m_TermCond != Term::Continue)
			break;
	}

	// No matter if we terminate now or not, loop through all Diagnostics to update them
	for (const auto &d : m_AllDiagnostics)
	{
		d->UpdateData();
	}

	return m_TermCond;
}

/**
 * @brief Get the current termination condition
 *
 * @return Term
 */
Term Geodesic::getTermCondition() const
{
	return m_TermCond;
}

/**
 * @brief Get the current position
 *
 * @return Point
 */
Point Geodesic::getCurrentPos() const
{
	return m_CurrentPos;
}

/**
 * @brief Get the current velocity
 *
 * @return OneIndex
 */
OneIndex Geodesic::getCurrentVel() const
{
	return m_CurrentVel;
}

/**
 * @brief Get the current value of the affine parameter
 *
 * @return real
 */
real Geodesic::getCurrentLambda() const
{
	return m_curLambda;
}

/**
 * @brief Get the screen index
 *
 * @return ScreenIndex
 */
ScreenIndex Geodesic::getScreenIndex() const
{
	return m_ScreenIndex;
}

/**
 * @brief Get the complete output that should be written to the output files
 * @details There is one string more than the count of Diagnostics: one string per Diagnostic,
 *	PLUS the first string is the screen index.
 *
 * @return std::vector<std::string>
 */
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
	std::string strScreenIndex{""};
	for (int i = 0; i < m_ScreenIndex.size(); ++i)
	{
		// We don't use the toString() function to avoid extraneous parentheses and commas therein
		strScreenIndex += std::to_string(m_ScreenIndex[i]) + " ";
	}
	theOutput.push_back(std::move(strScreenIndex));

	// The rest of the strings are the output strings as given by each of the Diagnostics
	for (const auto &d : m_AllDiagnostics)
	{
		theOutput.push_back(std::move(d->getFullDataStr()));
	}

	return theOutput;
}

/**
 * @brief Get the "value" (from the Diagnostic that was set to the value Diagnostic) that is associated to the Geodesic
 * @details Will be used to determine "distance" between Geodesics which is used in Mesh refinement.
 *
 * @return std::vector<real>
 */
std::vector<real> Geodesic::getDiagnosticFinalValue() const
{
	// The Geodesic should have terminated if this is called!
	if (m_TermCond == Term::Continue)
		ScreenOutput("Geodesic not terminated yet but getDiagnosticFinalValue() is called!", OutputLevel::Level_0_WARNING);

	// The diagnostic that contributes the value is always at the first position in the Diagnostic array!
	return m_AllDiagnostics[0]->getFinalDataVal();
}
