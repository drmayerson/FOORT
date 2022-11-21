#include"Geodesic.h"
#include"Metric.h"

std::string Source::GetDescriptionString() const
{
	return "Source (no override description specified)";
}

OneIndex NoSource::getSource(Point pos, OneIndex vel) const
{
	// no rhs for the geodesic equation
	return OneIndex{ 0,0,0,0 };
}

std::string NoSource::GetDescriptionString() const
{
	return "No source";
}

///////////////////////////////////


Term Geodesic::Update()
{
	// Integrate one step!
	Point newpos{};
	OneIndex newvel{};
	real step{};
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

Term Geodesic::GetTermCondition() const
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
	std::vector<std::string> theOutput{};
	theOutput.reserve(m_AllDiagnostics.size() + 1);

	// First position is screen index
	std::string strScreenIndex{ "" };
	for (int i=0; i<dimension-2; ++i)
	{
		strScreenIndex += std::to_string(m_ScreenIndex[i]) + " ";
	}
	theOutput.push_back(strScreenIndex);

	for (const auto& d : m_AllDiagnostics)
	{
		theOutput.push_back(d->getFullData());
	}

	return theOutput;
}

std::vector<real> Geodesic::GetDiagnosticFinalValue() const
{
	// The diagnostic that contributes the value is always at the first position in the Diagnostic array!
	return m_AllDiagnostics[0]->getFinalDataVal();
}