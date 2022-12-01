#include"Metric.h" // we are defining the Metric functions here

#include"InputOutput.h" // needed for ScreenOutput()
#include"Integrators.h" // needed for the constant DERIVATIVE_hval

#include<cmath> // needed for sqrt() and sin() etc (only on Linux)

/// <summary>
/// Metric (abstract base class) functions
/// </summary>

// Christoffel symbols of the metric (indices up, down, down)
ThreeIndex Metric::getChristoffel_udd(const Point& p) const
{
	// Populate metric derivatives with index down. Only evaluates numerical derivative of metric for a given coordinate
	// if the metric does not have a symmetry in that coordinate (otherwise derivative vanishes)
	// (exploiting symmetries this way speeds up computations considerably!)
	ThreeIndex metric_dd_der{};
	// Helper function that returns a bool true/false if the coordinate is a symmetry yes/no
	auto HasSym = [this](int theCoord) { return std::find(m_Symmetries.begin(), m_Symmetries.end(), theCoord) != m_Symmetries.end(); };
	for (int coord = 0; coord < dimension; ++coord)
	{
		if (!HasSym(coord))
		{
			// The metric does not have a symmetry in the coordinate coord, so we calculate the derivative of the metric
			// wrt this coordinate by central difference
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

// Riemann tensor (indices up, down, down, down)
FourIndex Metric::getRiemann_uddd(const Point& p) const
{
	// TO IMPLEMENT!
	ScreenOutput("Called Riemann at" + toString(p));

	return {};
}

// Kretschmann scalar (Riem^2)
real Metric::getKretschmann(const Point& p) const
{
	// TO IMPLEMENT!
	ScreenOutput("Called Kretschmann at" + toString(p));

	return 0;
}

// Generic description string
std::string Metric::getFullDescriptionStr() const
{
	return "Metric (no override description specified)";
}




/// <summary>
/// SphericalHorizonMetric functions
/// </summary>

// Constructor, to be called with the horizon radius and a bool indicating whether we are using a logarithmic radial scale
SphericalHorizonMetric::SphericalHorizonMetric(real HorizonRadius, bool rLogScale)
	: m_rLogScale{ rLogScale }, m_HorizonRadius{ HorizonRadius }
{ }

// Getter for horizon radius
real SphericalHorizonMetric::getHorizonRadius() const
{
	return m_HorizonRadius;
}

// Getter for radial log scale
bool SphericalHorizonMetric::getrLogScale() const
{
	return m_rLogScale;
}


/// <summary>
/// KerrMetric functions
/// </summary>

// Constructor, must be passed the Kerr a parameter and whether we are using a logarithmic radial scale
KerrMetric::KerrMetric(real aParam, bool rLogScale)
	: m_aParam{ aParam }, 
	SphericalHorizonMetric(1 + sqrt(1 - aParam * aParam), rLogScale) // initialize base class with horizon radius and rLogScale
{
	// Make sure we are in four spacetime dimensions
	if constexpr (dimension != 4)
	{
		ScreenOutput("Kerr is only defined in four dimensions!", OutputLevel::Level_0_WARNING);
	}
	// Kerr has a Killing vector along t and phi, so we initialize the symmetries accordingly
	m_Symmetries = { 0,3 };
}

// Kerr metric getter, indices down
TwoIndex KerrMetric::getMetric_dd(const Point& p) const
{
	// If logscale is turned on, then the first coordinate is actually u = log(r), so r = e^u
	real r = m_rLogScale ? exp(p[1]) : p[1];

	// Shorthands
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

// Kerr metric getter, indices up
TwoIndex KerrMetric::getMetric_uu(const Point& p) const
{
	// If logscale is turned on, then the first coordinate is actually u = log(r), so r = e^u
	real r = m_rLogScale ? exp(p[1]) : p[1];

	// Shorthands
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

// Kerr description string; also gives a parameter value and whether we are using logarithmic radial coordinate
std::string KerrMetric::getFullDescriptionStr() const
{
	return "Kerr (a = " + std::to_string(m_aParam) + ", " + (m_rLogScale ? "using logarithmic r coord" : "using normal r coord") + ")";
}


/// <summary>
/// FlatSpaceMetric functions
/// </summary>

// Basic constructor (no arguments necessary)
FlatSpaceMetric::FlatSpaceMetric()
{
	// Make sure we are in four spacetime dimensions
	if constexpr (dimension != 4)
	{
		ScreenOutput("FlatSpaceMetric is only defined in four dimensions!", OutputLevel::Level_0_WARNING);
	}

	// Killing vectors along t and phi (other Killing vectors of flat space not explicit in spherical coords)
	m_Symmetries = { 0,3 };
}

// Flat metric getter, indices down
TwoIndex FlatSpaceMetric::getMetric_dd(const Point& p) const
{
	// Flat metric in spherical coordinates
	return TwoIndex{ {{-1, 0,0,0}, {0,1,0,0}, {0,0,p[1]*p[1],0},{0,0,0,p[1] * p[1]*sin(p[2])*sin(p[2])}} };
}

// Flat metric getter, indices up
TwoIndex FlatSpaceMetric::getMetric_uu(const Point& p) const
{
	// Flat metric in spherical coordinates
	return TwoIndex{ {{-1, 0,0,0}, {0,1,0,0}, {0,0,1/(p[1] * p[1]),0},{0,0,0,1/(p[1] * p[1] * sin(p[2]) * sin(p[2]))}} };
}

// Description string for flat space
std::string FlatSpaceMetric::getFullDescriptionStr() const
{
	return "Flat space";
}



//// (New Metric classes can define their member functions here)