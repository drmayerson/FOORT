#include"Geometry.h"

#include"InputOutput.h"
#include"Integrators.h"

#include"Metric.h"

#include<cassert>

/// <summary>
/// Metric functions
/// </summary>

ThreeIndex Metric::getChristoffel_udd(const Point& p) const
{
	//ScreenOutput("Called Christoffel at" + toString(p),OutputLevel::Level_4_DEBUG);

	// Populate metric derivatives with index down. Only evaluates numerical derivative of metric for a given coordinate
	// if the metric does not have a symmetry in that coordinate (otherwise derivative vanishes)
	ThreeIndex metric_dd_der{};
	auto HasSym = [this](int theCoord) { return std::find(m_Symmetries.begin(), m_Symmetries.end(), theCoord) != m_Symmetries.end(); };
	for (int coord = 0; coord < dimension; ++coord)
	{
		if (!HasSym(coord))
		{
			Point pShift{};
			pShift[coord] = DERIVATIVE_hval;
			metric_dd_der[coord] = (getMetric_dd(p + pShift) - getMetric_dd(p - pShift)) / (2 * DERIVATIVE_hval);
		}
	}

	// Metric with index up
	TwoIndex metric_uu{ getMetric_uu(p) };

	// Construct Christoffel symbol Gamma^{\mu}_{\nu\rho}
	ThreeIndex theChristoffel{};
	for (int mu = 0; mu < dimension; ++mu)
	{
		for (int nu = 0; nu < dimension; ++nu)
		{
			for (int rho = 0; rho < dimension; ++rho)
			{
				for (int sigma = 0; sigma < dimension; ++sigma)
				{
					theChristoffel[mu][nu][rho] += 1.0/2 * metric_uu[mu][sigma] *
						(metric_dd_der[nu][rho][sigma] + metric_dd_der[rho][nu][sigma] - metric_dd_der[sigma][nu][rho]);
				}
			}
		}
	}

	return theChristoffel;
}

FourIndex Metric::getRiemann_uddd(const Point& p) const
{
	ScreenOutput("Called Riemann at" + toString(p));

	return {};
}

real Metric::getKretschmann(const Point& p) const
{
	ScreenOutput("Called Kretschmann at" + toString(p));

	return 0;
}




/// <summary>
/// SphericalHorizonMetric functions
/// </summary>

SphericalHorizonMetric::SphericalHorizonMetric(real HorizonRadius, real AtHorizonEps, bool rLogScale)
	: m_rLogScale{ rLogScale }, m_HorizonRadius{ HorizonRadius }, m_AtHorizonEps{AtHorizonEps}
{
	//ScreenOutput("SphericalHorizon constructor: " + std::to_string(m_HorizonRadius), OutputLevel::Level_4_DEBUG);
}

Term SphericalHorizonMetric::InternalTerminate(const Point& p) const
{
	Term ret = Term::Continue;
	//ScreenOutput(toString(p),OutputLevel::Level_4_DEBUG);
	//ScreenOutput(std::to_string(m_HorizonRadius), OutputLevel::Level_4_DEBUG);

	// Check to see if radius is almost at horizon; if we are using a logarithmic r scale (u=log(r)) then first exponentiate to get
	// true radius
	real r = m_rLogScale ? exp(p[1]) : p[1];
	if (r < m_HorizonRadius * (1 + m_AtHorizonEps))
	{
		ret = Term::Horizon;
	}
		
	return ret;
}


/// <summary>
/// KerrMetric functions
/// </summary>


KerrMetric::KerrMetric(real aParam, real atHorizonEps, bool rLogScale)
	: m_aParam{ aParam }, 
	SphericalHorizonMetric(1 + sqrt(1 - aParam * aParam), atHorizonEps, rLogScale)
{
	assert(dimension == 4 && "Cannot construct Kerr metric in spacetime dimension other than 4!");

	// Kerr has a Killing vector along t and phi
	m_Symmetries = { 0,3 };

	ScreenOutput("Kerr metric constructed with a = " + std::to_string(m_aParam)
		+ "; horizon at: " + std::to_string(m_HorizonRadius) + "; using log(r): " + std::to_string(m_rLogScale)
		+ "; epsilon at horizon: " + std::to_string(m_AtHorizonEps) + ".", OutputLevel::Level_4_DEBUG);
	
}

TwoIndex KerrMetric::getMetric_dd(const Point& p) const
{
	// ScreenOutput("Kerr metric dd at " + toString(p));

	// If logscale is turned on, then the first coordinate is actually u = log(r), so r = e^u
	real r = m_rLogScale ? exp(p[1]) : p[1];

	real theta = p[2];
	real sint = sin(theta);
	real cost = cos(theta);
	real sigma = r * r + m_aParam * m_aParam * cost * cost;
	real delta = r * r + m_aParam * m_aParam - 2. * r;
	real A_ = (r * r + m_aParam * m_aParam) * (r * r + m_aParam * m_aParam)
		- delta * m_aParam * m_aParam * sint * sint;

	// Covariant metric elements
	real g00 = -(1. - 2. * r / sigma);
	real g11 = sigma / delta;
	real g22 = sigma;
	real g33 = A_ / sigma * sint * sint;
	real g03 = -2. * m_aParam * r * sint * sint / sigma;

	// If the log scale is set on, the true coordinate we are calculating the metric in is u = log(r), so dr = r du
	if (m_rLogScale)
	{
		g11 *= (r * r);
	}

	return TwoIndex{ {{g00, 0,0, g03 }, {0,g11,0,0}, {0,0,g22,0},{g03,0,0,g33}} };
}


TwoIndex KerrMetric::getMetric_uu(const Point& p) const
{
	//ScreenOutput("Kerr metric uu at " + toString(p));

	// If logscale is turned on, then the first coordinate is actually u = log(r), so r = e^u
	real r = m_rLogScale ? exp(p[1]) : p[1];

	real theta = p[2];
	real sint = sin(theta);
	real cost = cos(theta);
	real sigma = r * r + m_aParam * m_aParam * cost * cost;
	real delta = r * r + m_aParam * m_aParam - 2. * r;
	real A_ = (r * r + m_aParam * m_aParam) * (r * r + m_aParam * m_aParam) - delta * m_aParam * m_aParam *
		sint * sint;

	// Contravariant metric elements
	real g00=  -A_ / (sigma * delta);
	real g11 = delta / sigma;
	real g22 = 1. / sigma;
	real g33= (delta - m_aParam * m_aParam * sint * sint) /
		(sigma * delta * sint * sint);
	real g03 = -2. * m_aParam * r / (sigma * delta);

	// If the log scale is set on, the true coordinate we are calculating the metric in is u = log(r), so , so dr = r du
	if (m_rLogScale)
	{
		g11 *= 1.0 / (r * r);
	}
	
	return TwoIndex{ {{g00, 0,0, g03 }, {0,g11,0,0}, {0,0,g22,0},{g03,0,0,g33}} };
}


/// <summary>
/// FlatSpaceMetric functions
/// </summary>

FlatSpaceMetric::FlatSpaceMetric()
{
	assert(dimension == 4 && "Flat space metric only defined in 4 dimensions!");

	// Killing vectors along t and phi (other Killing vectors of flat space not explicit in spherical coords)
	m_Symmetries = { 0,3 };
}

TwoIndex FlatSpaceMetric::getMetric_dd(const Point& p) const
{
	return TwoIndex{ {{-1, 0,0,0}, {0,1,0,0}, {0,0,p[1]*p[1],0},{0,0,0,p[1] * p[1]*sin(p[2])*sin(p[2])}} };
}

TwoIndex FlatSpaceMetric::getMetric_uu(const Point& p) const
{
	return TwoIndex{ {{-1, 0,0,0}, {0,1,0,0}, {0,0,1/(p[1] * p[1]),0},{0,0,0,1/(p[1] * p[1] * sin(p[2]) * sin(p[2]))}} };

}

Term FlatSpaceMetric::InternalTerminate(const Point& p) const
{
	return Term::Continue;
}