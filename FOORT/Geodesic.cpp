#include"Geodesic.h"
#include"Metric.h"

OneIndex NoSource::getSource(Point pos, OneIndex vel) const
{
	// no rhs for the geodesic equation
	return OneIndex{ 0,0,0,0 };
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

	// Does metric want termination (due to e.g. horizon)?
	m_TermCond = m_theMetric->InternalTerminate(m_CurrentPos);

	// If metric does not want termination, check all other Terminations
	if (m_TermCond == Term::Continue)
	{
		for (const auto& t : m_AllTerminations)
		{
			m_TermCond = t->CheckTermination(*this);
			if (m_TermCond != Term::Continue)
				break;
		}
	}

	// No matter if we terminate now or not, loop through all Diagnostics to update them
	for (const auto& d : m_AllDiagnostics)
	{
		d->UpdateData(*this);
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


//DiagnosticCopyVector Geodesic::getDiagnostics() const
//{
//	DiagnosticCopyVector thecopies;
//	thecopies.reserve(m_AllDiagnostics.size());
//	for (const auto& uniquediag : m_AllDiagnostics)
//	{
//		thecopies.push_back(uniquediag.get());
//	}
//	return thecopies;
//}


std::string Geodesic::getDiagnosticOutputStr() const
{
	std::string theOutput{ "" };

	for (int i=0; i<dimension-2; ++i)
	{
		theOutput += std::to_string(m_ScreenIndex[i]) + " ";
	}

	for (const auto& d : m_AllDiagnostics)
	{
		theOutput += "X " + d->getFullData();
	}

	return theOutput;
}

std::vector<real> Geodesic::GetDiagnosticFinalValue() const
{
	// The diagnostic that contributes the value is always at the first position in the Diagnostic array!
	return m_AllDiagnostics[0]->getFinalDataVal();
}