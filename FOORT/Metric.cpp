#include "Metric.h" // we are defining the Metric functions here

#include "InputOutput.h" // needed for ScreenOutput()
#include "Integrators.h" // needed for Integrators::Derivative_hval

#include <cmath> // needed for sqrt() and sin() etc (only on Linux)
#include <algorithm> // needed for std::find

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
			pShift[coord] = Integrators::Derivative_hval;
			metric_dd_der[coord] = (getMetric_dd(p + pShift) - getMetric_dd(p - pShift)) / (2 * Integrators::Derivative_hval);
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

	// Check on parameters
	if (m_aParam * m_aParam > 1.0)
		ScreenOutput("Kerr metric a parameter given (" + std::to_string(m_aParam) + ") is not within the allowed range -1 < a < 1!",
			OutputLevel::Level_0_WARNING);

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


/// <summary>
/// RasheedLarsenMetric functions
/// </summary>

// Constructor, must be passed the four RL parameters and whether we are using a logarithmic radial scale
// We rescale all parameters by the mass to end up with a M = 1 BH
RasheedLarsenMetric::RasheedLarsenMetric(real mParam, real aParam, real pParam, real qParam, bool rLogScale)
	: m_aParam{ aParam / ((pParam + qParam) / 4.0) },
	m_mParam{ mParam / ((pParam + qParam) / 4.0) },
	m_pParam{ pParam / ((pParam + qParam) / 4.0) },
	m_qParam{ qParam / ((pParam + qParam) / 4.0) },
	// initialize base class with horizon radius and rLogScale
	SphericalHorizonMetric( ( mParam + sqrt(mParam * mParam - aParam * aParam) ) / ((pParam + qParam) / 4.0), rLogScale)
{
	// Make sure we are in four spacetime dimensions
	if constexpr (dimension != 4)
	{
		ScreenOutput("Rasheed-Larsen is only defined in four dimensions!", OutputLevel::Level_0_WARNING);
	}

	// Check to see if the parameter values are within the correct ranges
	if (m_pParam - 2.0 * m_mParam < 0.0
		|| m_qParam  - 2.0 * m_mParam  < 0.0
		|| m_aParam*m_aParam > m_mParam*m_mParam
		|| m_mParam < 0.0)
	{
		ScreenOutput("Rasheed-Larsen parameters outside of allowed range! Parameters given: m = " + std::to_string(m_mParam)
			+ ", a = " + std::to_string(m_aParam) + ", p = " + std::to_string(m_pParam) + ", q = " + std::to_string(m_qParam) + ".",
			OutputLevel::Level_0_WARNING);
	}

	// Rasheed-Larsen has a Killing vector along t and phi, so we initialize the symmetries accordingly
	m_Symmetries = { 0,3 };
}

TwoIndex RasheedLarsenMetric::getMetric_dd(const Point& p) const
{
	// If logscale is turned on, then the first coordinate is actually u = log(r), so r = e^u
	real r = m_rLogScale ? exp(p[1]) : p[1];

	real r2 = r * r;

	real theta = p[2];
	real sint = sin(theta);
	real cost = cos(theta);
	real sint2 = sint * sint;
	real cost2 = cost * cost;

	real delta = r2 + m_aParam * m_aParam - 2. * r * m_mParam;
	real H3 = r2 - 2. *  r * m_mParam  +  m_aParam * m_aParam * cost2;
	real Bp = sqrt(m_pParam * m_qParam) * m_aParam * sint2 * (((m_pParam * m_qParam + 4. * m_mParam * m_mParam) * r
		- m_mParam * (m_pParam - 2. * m_mParam) * (m_qParam - 2. * m_mParam)) / (2. * m_mParam * (m_pParam + m_qParam) * H3));
		
	real H1 = r2 + m_aParam * m_aParam * cost2 + r * (m_pParam - 2. * m_mParam)
		+ (m_pParam / (m_pParam + m_qParam)) * ((m_pParam - 2. * m_mParam) * (m_qParam - 2. * m_mParam) / 2.)
		- (m_pParam / (2. * m_mParam * (m_pParam + m_qParam))) * sqrt((m_pParam * m_pParam - 4. * m_mParam * m_mParam)
			* (m_qParam * m_qParam - 4. * m_mParam * m_mParam)) * m_aParam * cost;
	real H2 = r2 + m_aParam * m_aParam * cost2 + r * (m_qParam - 2. * m_mParam)
		+ (m_qParam / (m_pParam + m_qParam)) * ((m_pParam - 2. * m_mParam) * (m_qParam - 2. * m_mParam) / 2.)
		+ (m_qParam / (2. * m_mParam * (m_pParam + m_qParam))) * sqrt((m_pParam * m_pParam - 4. * m_mParam * m_mParam)
			* (m_qParam * m_qParam - 4. * m_mParam * m_mParam)) * m_aParam * cost;

	real sqH1H2 = sqrt(H1 * H2);

	// Covariant metric elements
	real g00 = -H3 / sqH1H2;
	real g11 = sqH1H2 / delta;
	real g22 = sqH1H2;
	real g33 = -(H3 * Bp * Bp) / sqH1H2 + (sqH1H2 * delta * sint2) / H3;
	real g03 = -(H3 * Bp) / sqH1H2;

	// If the log scale is set on, the true coordinate we are calculating the metric in is u = log(r), so dr = r du
	if (m_rLogScale)
	{
		g11 *= (r * r);
	}

	return TwoIndex{ {{g00, 0,0, g03 }, {0,g11,0,0}, {0,0,g22,0},{g03,0,0,g33}} };
}

TwoIndex RasheedLarsenMetric::getMetric_uu(const Point& p) const
{
	// If logscale is turned on, then the first coordinate is actually u = log(r), so r = e^u
	real r = m_rLogScale ? exp(p[1]) : p[1];

	real r2 = r * r;

	real theta = p[2];
	real sint = sin(theta);
	real cost = cos(theta);
	real sint2 = sint * sint;
	real cost2 = cost * cost;

	real delta = r2 + m_aParam * m_aParam - 2. * r * m_mParam;
	real H3 = r2 - 2. * r * m_mParam + m_aParam * m_aParam * cost2;
	real Bp = sqrt(m_pParam * m_qParam) * m_aParam * sint2 * (((m_pParam * m_qParam + 4. * m_mParam * m_mParam) * r
		- m_mParam * (m_pParam - 2. * m_mParam) * (m_qParam - 2. * m_mParam)) / (2. * m_mParam * (m_pParam + m_qParam) * H3));

	real H1 = r2 + m_aParam * m_aParam * cost2 + r * (m_pParam - 2. * m_mParam)
		+ (m_pParam / (m_pParam + m_qParam)) * ((m_pParam - 2. * m_mParam) * (m_qParam - 2. * m_mParam) / 2.)
		- (m_pParam / (2. * m_mParam * (m_pParam + m_qParam))) * sqrt((m_pParam * m_pParam - 4. * m_mParam * m_mParam)
			* (m_qParam * m_qParam - 4. * m_mParam * m_mParam)) * m_aParam * cost;
	real H2 = r2 + m_aParam * m_aParam * cost2 + r * (m_qParam - 2. * m_mParam)
		+ (m_qParam / (m_pParam + m_qParam)) * ((m_pParam - 2. * m_mParam) * (m_qParam - 2. * m_mParam) / 2.)
		+ (m_qParam / (2. * m_mParam * (m_pParam + m_qParam))) * sqrt((m_pParam * m_pParam - 4. * m_mParam * m_mParam)
			* (m_qParam * m_qParam - 4. * m_mParam * m_mParam)) * m_aParam * cost;

	real sqH1H2 = sqrt(H1 * H2);

	// Contravariant metric elements
	real g00 = ((H3 * H3 * Bp * Bp) / sint2 - H1 * H2 * delta) / (sqH1H2 * H3 * delta);
	real g11 = delta / sqH1H2;
	real g22 = 1. / sqH1H2;
	real g33 = H3 / (sqH1H2 * delta * sint2);
	real g03 = -(H3 * Bp) / (sqH1H2 * delta * sint2);

	// If the log scale is set on, the true coordinate we are calculating the metric in is u = log(r), so , so dr = r du
	if (m_rLogScale)
	{
		g11 *= 1.0 / (r * r);
	}

	return TwoIndex{ {{g00, 0,0, g03 }, {0,g11,0,0}, {0,0,g22,0},{g03,0,0,g33}} };
}

// Rasheed-Larsen description string; also gives a parameter value and whether we are using logarithmic radial coordinate
std::string RasheedLarsenMetric::getFullDescriptionStr() const
{
	return "Rasheed-Larsen (m = " + std::to_string(m_mParam) + ", a = " + std::to_string(m_aParam) 
		+ ", p = " + std::to_string(m_pParam) + ", q = " + std::to_string(m_qParam) + ", "
		+ (m_rLogScale ? "using logarithmic r coord" : "using normal r coord") + ")";
}


/// <summary>
/// JohannsenMetric functions (implementation by Seppe Staelens)
/// </summary>

// Constructor, must be passed the five Joh parameters and whether we are using a logarithmic radial scale
// We always have a M = 1 BH
JohannsenMetric::JohannsenMetric(real aParam, real alpha13Param, real alpha22Param, real alpha52Param, real eps3Param, bool rLogScale)
	: m_aParam{ aParam },
	m_alpha13Param{ alpha13Param },
	m_alpha22Param{ alpha22Param },
	m_alpha52Param{ alpha52Param },
	m_eps3Param{ eps3Param },
	// initialize base class with Kerr horizon radius and rLogScale
	SphericalHorizonMetric(1 + sqrt(1 - aParam * aParam), rLogScale) // initialize base class with horizon radius and rLogScale
{
	// Make sure we are in four spacetime dimensions
	if constexpr (dimension != 4)
	{
		ScreenOutput("Johannsen is only defined in four dimensions!", OutputLevel::Level_0_WARNING);
	}

	// Check to see if the parameter values are within the correct ranges
	real horizon_radius = 1.0 + sqrt(1.0 - m_aParam * m_aParam);
	if (m_aParam * m_aParam > 1.0
		|| m_alpha52Param <= -horizon_radius * horizon_radius
		|| m_eps3Param <= -horizon_radius * horizon_radius * horizon_radius
		|| m_alpha13Param <= -horizon_radius * horizon_radius * horizon_radius)
	{
		ScreenOutput("Johannsen metric parameters outside of allowed range! Parameters given: a = " + std::to_string(m_aParam) + ", alpha13 = " + std::to_string(m_alpha13Param)
			+ ", alpha22 = " + std::to_string(m_alpha22Param) + ", alpha52 = " + std::to_string(m_alpha52Param) + ", epsilon3 = " + std::to_string(m_eps3Param) + ".",
			OutputLevel::Level_0_WARNING);
	}

	// Johannsen has a Killing vector along t and phi, so we initialize the symmetries accordingly
	m_Symmetries = { 0,3 };
}

TwoIndex JohannsenMetric::getMetric_dd(const Point& p) const
{
	// If logscale is turned on, then the first coordinate is actually u = log(r), so r = e^u
	real r = m_rLogScale ? exp(p[1]) : p[1];

	real theta = p[2];
	real sint = sin(theta);
	real cost = cos(theta);

	real A1 = 1. + m_alpha13Param / (r * r * r);
	real A2 = 1. + m_alpha22Param / (r * r);
	real A5 = 1. + m_alpha52Param / (r * r);
	real eps_f = m_eps3Param / r;

	real rho2 = r * r + m_aParam * m_aParam * cost * cost + eps_f; // Sigma tilde in paper
	real delta = r * r + m_aParam * m_aParam - 2. * r;

	// Covariant metric elements
	real g00 = -rho2 * (delta - m_aParam * m_aParam * A2 * A2 * sint * sint) / pow((r * r + m_aParam * m_aParam) * A1 - m_aParam * m_aParam * A2 * sint * sint, 2.);
	real g11 = rho2 / (delta * A5);
	real g22 = rho2;
	real g33 = rho2 * sint * sint * (pow((r * r + m_aParam * m_aParam) * A1, 2.) - m_aParam * m_aParam * delta * sint * sint) / pow((r * r + m_aParam * m_aParam) * A1 - m_aParam * m_aParam * A2 * sint * sint, 2.);
	real g03 = -m_aParam * ((r * r + m_aParam * m_aParam) * A1 * A2 - delta) * rho2 * sint * sint / pow((r * r + m_aParam * m_aParam) * A1 - m_aParam * m_aParam * A2 * sint * sint, 2.);

	// If the log scale is set on, the true coordinate we are calculating the metric in is u = log(r), so dr = r du
	if (m_rLogScale)
	{
		g11 *= (r * r);
	}

	return TwoIndex{ {{g00, 0,0, g03 }, {0,g11,0,0}, {0,0,g22,0},{g03,0,0,g33}} };
}

TwoIndex JohannsenMetric::getMetric_uu(const Point& p) const
{
	// If logscale is turned on, then the first coordinate is actually u = log(r), so r = e^u
	real r = m_rLogScale ? exp(p[1]) : p[1];

	real theta = p[2];
	real sint = sin(theta);
	real cost = cos(theta);

	real A1 = 1. + m_alpha13Param / (r * r * r);
	real A2 = 1. + m_alpha22Param / (r * r);
	real A5 = 1. + m_alpha52Param / (r * r);
	real eps_f = m_eps3Param / r;

	real rho2 = r * r + m_aParam * m_aParam * cost * cost + eps_f; // Sigma tilde in paper
	real delta = r * r + m_aParam * m_aParam - 2. * r;

	// Contravariant metric elements
	real g00 = (-1. * pow((r * r + m_aParam * m_aParam) * A1, 2.) + m_aParam * m_aParam * delta * sint * sint) / (delta * rho2);
	real g11 = delta * A5 / rho2;
	real g22 = 1. / rho2;
	real g33 = (-m_aParam * m_aParam * A2 * A2 * sint * sint + delta) / (delta * rho2 * sint * sint);
	real g03 = -m_aParam * (A2 * A1 * (r * r + m_aParam * m_aParam) - delta) / (delta * rho2);

	// If the log scale is set on, the true coordinate we are calculating the metric in is u = log(r), so , so dr = r du
	if (m_rLogScale)
	{
		g11 *= 1.0 / (r * r);
	}

	return TwoIndex{ {{g00, 0,0, g03 }, {0,g11,0,0}, {0,0,g22,0},{g03,0,0,g33}} };
}

// Johannsen description string; also gives a parameter value and whether we are using logarithmic radial coordinate
std::string JohannsenMetric::getFullDescriptionStr() const
{
	return "Johannsen (a = " + std::to_string(m_aParam) + ", alpha13 = " + std::to_string(m_alpha13Param)
		+ ", alpha22 = " + std::to_string(m_alpha22Param) + ", alpha52 = " + std::to_string(m_alpha52Param) + ", epsilon3 = " + std::to_string(m_eps3Param) + ", "
		+ (m_rLogScale ? "using logarithmic r coord" : "using normal r coord") + ")";
}

/// <summary>
/// MankoNovikovMetric functions (implementation by Seppe Staelens)
/// </summary>

// Constructor, must be passed the two MaNo parameters and whether we are using a logarithmic radial scale
// We always have a M = 1 BH; the parameter a gives the angular momentum a = J/M^2
// The horizon is at r = M + k
MankoNovikovMetric::MankoNovikovMetric(real aParam, real alpha3Param, bool rLogScale)
	: m_aParam{ aParam },
	m_alpha3Param{ alpha3Param },
	m_alphaParam{ (aParam == 0.0) ? 0.0 : (-1. + sqrt(1. - aParam * aParam)) / aParam },
	m_kParam{ sqrt(1. - aParam * aParam) },
	// initialize base class with Kerr horizon radius and rLogScale
	SphericalHorizonMetric(1 + sqrt(1. - aParam * aParam), rLogScale) // initialize base class with horizon radius and rLogScale
{
	// Make sure we are in four spacetime dimensions
	if constexpr (dimension != 4)
	{
		ScreenOutput("Manko-Novikov is only defined in four dimensions!", OutputLevel::Level_0_WARNING);
	}

	// Check to see if the parameter values are within the correct ranges
	// Not applicable to this metric

	// Manko-Novikov BH has a Killing vector along t and phi, so we initialize the symmetries accordingly
	m_Symmetries = { 0,3 };
}

TwoIndex MankoNovikovMetric::getMetric_dd(const Point& p) const
{

	// spherical coordinates
	// If logscale is turned on, then the first coordinate is actually u = log(r), so r = e^u
	real r = m_rLogScale ? exp(p[1]) : p[1];

	real theta = p[2];
	real sint = sin(theta);
	real cost = cos(theta);

	// auxiliary functions
	real xx = (r - 1.) / m_kParam;
	real yy = cost;

	real R = sqrt(xx * xx + yy * yy - 1.);

	real P1 = xx * yy / R;
	real P2 = 0.5 * (3. * xx * xx * yy * yy / (R * R) - 1.);
	real P3 = 0.5 * (5. * pow(xx * yy / R, 3.) - 3. * xx * yy / R);
	real P4 = 0.125 * (35. * pow(xx * yy / R, 4.) - 30. * xx * xx * yy * yy / (R * R) + 3.);

	real aa = -m_alphaParam * exp(2. * m_alpha3Param * (1. - (xx - yy) * (1. / R + P1 / (R * R) + P2 / (R * R * R) + P3 / (R * R * R * R))));
	real bb = m_alphaParam * exp(2. * m_alpha3Param * (-1. + (xx + yy) * (1. / R - P1 / (R * R) + P2 / (R * R * R) - P3 / (R * R * R * R))));

	real AA = (xx * xx - 1.) * (1. + aa * bb) * (1. + aa * bb) - (1. - yy * yy) * (bb - aa) * (bb - aa);
	real BB = pow((xx + 1. + (xx - 1.) * aa * bb), 2.) + pow(((1. + yy) * aa + (1. - yy) * bb), 2.);
	real CC = (xx * xx - 1.) * (1. + aa * bb) * (bb - aa - yy * (aa + bb)) + (1. - yy * yy) * (bb - aa) * (1. + aa * bb + xx * (1. - aa * bb));

	real psi = m_alpha3Param * P3 / (R * R * R * R);
	real gamma_prime = 2. * m_alpha3Param * m_alpha3Param * (P4 * P4 - P3 * P3) / (pow(R, 8.)) + 2. * m_alpha3Param * (-yy + xx * P1 / R - yy * P2 / (R * R) + xx * P3 / (R * R * R)) / R;

	real f = exp(2. * psi) * AA / BB;
	real exp_2gamma = exp(2. * gamma_prime) * AA / ((xx * xx - yy * yy) * (1. - m_alphaParam * m_alphaParam) * (1. - m_alphaParam * m_alphaParam));
	real omega = 2. * m_kParam * exp(-2. * psi) * CC / AA - 4. * m_kParam * m_alphaParam / (1. - m_alphaParam * m_alphaParam);

	real rho_sq = (r - 1.) * (r - 1.) - m_kParam * m_kParam * cost * cost;
	real delta = (r - 1.) * (r - 1.) - m_kParam * m_kParam;

	// covariant metric components
	real g00 = -f;
	real g22 = exp_2gamma * rho_sq / f;
	real g11 = g22 / delta;
	real g33 = -f * omega * omega + delta * sint * sint / f;
	real g03 = omega * f;

	// If the log scale is set on, the true coordinate we are calculating the metric in is u = log(r), so dr = r du
	if (m_rLogScale)
	{
		g11 *= (r * r);
	}

	return TwoIndex{ {{g00, 0,0, g03 }, {0,g11,0,0}, {0,0,g22,0},{g03,0,0,g33}} };
}

//done up to here

TwoIndex MankoNovikovMetric::getMetric_uu(const Point& p) const
{
	// spherical coordinates
	// If logscale is turned on, then the first coordinate is actually u = log(r), so r = e^u
	real r = m_rLogScale ? exp(p[1]) : p[1];

	real theta = p[2];
	real sint = sin(theta);
	real cost = cos(theta);

	// auxiliary functions
	real xx = (r - 1.) / m_kParam;
	real yy = cost;

	real R = sqrt(xx * xx + yy * yy - 1.);

	real P1 = xx * yy / R;
	real P2 = 0.5 * (3. * xx * xx * yy * yy / (R * R) - 1.);
	real P3 = 0.5 * (5. * pow(xx * yy / R, 3.) - 3. * xx * yy / R);
	real P4 = 0.125 * (35. * pow(xx * yy / R, 4.) - 30. * xx * xx * yy * yy / (R * R) + 3.);

	real aa = -m_alphaParam * exp(2. * m_alpha3Param * (1. - (xx - yy) * (1. / R + P1 / (R * R) + P2 / (R * R * R) + P3 / (R * R * R * R))));
	real bb = m_alphaParam * exp(2. * m_alpha3Param * (-1. + (xx + yy) * (1. / R - P1 / (R * R) + P2 / (R * R * R) - P3 / (R * R * R * R))));

	real AA = (xx * xx - 1.) * (1. + aa * bb) * (1. + aa * bb) - (1. - yy * yy) * (bb - aa) * (bb - aa);
	real BB = pow((xx + 1. + (xx - 1.) * aa * bb), 2.) + pow(((1. + yy) * aa + (1. - yy) * bb), 2.);
	real CC = (xx * xx - 1.) * (1. + aa * bb) * (bb - aa - yy * (aa + bb)) + (1. - yy * yy) * (bb - aa) * (1. + aa * bb + xx * (1. - aa * bb));

	real psi = m_alpha3Param * P3 / (R * R * R * R);
	real gamma_prime = 2. * m_alpha3Param * m_alpha3Param * (P4 * P4 - P3 * P3) / (pow(R, 8.)) + 2. * m_alpha3Param * (-yy + xx * P1 / R - yy * P2 / (R * R) + xx * P3 / (R * R * R)) / R;

	real f = exp(2. * psi) * AA / BB;
	real exp_2gamma = exp(2. * gamma_prime) * AA / ((xx * xx - yy * yy) * (1. - m_alphaParam * m_alphaParam) * (1. - m_alphaParam * m_alphaParam));
	real omega = 2. * m_kParam * exp(-2. * psi) * CC / AA - 4. * m_kParam * m_alphaParam / (1. - m_alphaParam * m_alphaParam);

	real rho_sq = (r - 1.) * (r - 1.) - m_kParam * m_kParam * cost * cost;
	real delta = (r - 1.) * (r - 1.) - m_kParam * m_kParam;

	// metric functions
	real beta_d = omega * f;
	real gamma_dd = -f * omega * omega + delta * sint * sint / f;
	real beta_u = beta_d / gamma_dd;
	real alpha_sq = beta_u * beta_d + f;

	// contravariant metric components
	real g00 = -1. / alpha_sq;
	real g22 = f / (exp_2gamma * rho_sq);
	real g11 = delta * g22;
	real g33 = 1. / gamma_dd - beta_u * beta_u / alpha_sq;
	real g03 = beta_u / alpha_sq;

	// If the log scale is set on, the true coordinate we are calculating the metric in is u = log(r), so , so dr = r du
	if (m_rLogScale)
	{
		g11 *= 1.0 / (r * r);
	}

	return TwoIndex{ {{g00, 0,0, g03 }, {0,g11,0,0}, {0,0,g22,0},{g03,0,0,g33}} };
}

// Manko-Novikov description string; also gives a parameter value and whether we are using logarithmic radial coordinate
std::string MankoNovikovMetric::getFullDescriptionStr() const
{
	return "Manko-Novikov (a = " + std::to_string(m_aParam) + ", alpha3 = " + std::to_string(m_alpha3Param)
		+ ", " + (m_rLogScale ? "using logarithmic r coord" : "using normal r coord") + ")";
}


//// (New Metric classes can define their member functions here)