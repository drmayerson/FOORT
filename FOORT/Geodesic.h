#ifndef _FOORT_GEODESIC_H
#define _FOORT_GEODESIC_H

#include <string>
#include<memory>
#include<vector>

#include"Geometry.h"
#include"Metric.h"
#include"Diagnostics.h"
#include"Terminations.h"
#include"Integrators.h"

class Source
{
public:
	Source(const Metric* theMetric) : m_theMetric{ theMetric } {}
	virtual ~Source() = default;

	virtual OneIndex getSource(Point pos, OneIndex vel) const = 0;

	virtual std::string GetDescriptionString() const;

protected:
	const Metric* m_theMetric;
};

class NoSource : public Source
{
public:
	NoSource(const Metric* theMetric) : Source(theMetric) {}
	OneIndex getSource(Point pos, OneIndex vel) const override;

	std::string GetDescriptionString() const final;
};


class Geodesic
{
public:
	Geodesic(ScreenIndex scrindex, Point initpos, OneIndex initvel,
		Metric* theMetric, Source* theSource,
		DiagBitflag diagbit, DiagBitflag valdiagbit,
		TermBitflag termbit,GeodesicIntegratorFunc theIntegrator) : 
		m_ScreenIndex{ scrindex }, m_CurrentPos { initpos }, m_CurrentVel{ initvel },
		m_theMetric{ theMetric }, m_theSource{ theSource },
		m_AllDiagnostics{ CreateDiagnosticVector(diagbit,valdiagbit,this) }, m_AllTerminations{ CreateTerminationVector(termbit,this) },
		m_theIntegrator{ theIntegrator }
	{
		// Start: loop through all diagnostics to update at starting position
		for (const auto& d : m_AllDiagnostics)
		{
			d->UpdateData();
		}
	}


	Term Update();
	Term GetTermCondition() const;
	Point getCurrentPos() const;
	OneIndex getCurrentVel() const;
	real getCurrentLambda() const;

	std::vector<std::string> getAllOutputStr() const;
	std::vector<real> GetDiagnosticFinalValue() const;
protected:
	Term m_TermCond{Term::Continue};

	ScreenIndex m_ScreenIndex;
	Point m_CurrentPos;
	OneIndex m_CurrentVel;
	real m_curLambda{ 0.0 };

	Metric* m_theMetric;
	Source* m_theSource;
	GeodesicIntegratorFunc m_theIntegrator;
	DiagnosticUniqueVector m_AllDiagnostics;
	TerminationUniqueVector m_AllTerminations;
};

#endif
