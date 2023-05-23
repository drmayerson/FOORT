#include "Diagnostics_Emission.h" // We are defining EmissionModel and FluidVelocityModel functions here

#include "Integrators.h" // for Integrators::Derivative_hval, Integrators::delta_nodiv0

#include <cmath> // for asinh, exp, sqrt, fabs

/// <summary>
/// Emission model functions
/// </summary>

std::string EmissionModel::getFullDescriptionStr() const
{
	// Base class default string getter
	return "Unspecified emission model";
}

real GLMJohnsonSUEmission::GetEmission(const Point& p) const
{
	// Return the local emission according to the Johnson SU model used in GLM
	real r{ p[1] };
	real temp{ m_gamma + asinh((r - m_mu) / m_sigma) };
	real num{ exp(-1.0 / 2.0 * temp * temp) };
	real den{ sqrt((r - m_mu) * (r - m_mu) + m_sigma * m_sigma) };
	return num / den;
}

std::string GLMJohnsonSUEmission::getFullDescriptionStr() const
{
	// Description string
	return "GLM Johnson SU emission (mu = " + std::to_string(m_mu) + ", gamma = " + std::to_string(m_gamma)
		+ ", sigma = " + std::to_string(m_sigma) + ")";
}

/// <summary>
/// FluidVelocityModel functions
/// </summary>

std::string FluidVelocityModel::getFullDescriptionStr() const
{
	return "Unspecified fluid velocity model";
}

OneIndex GeneralCircularRadialFluid::GetFourVelocityd(const Point& p) const
{
	// Put point exactly on equator
	Point EquatPoint{ p[0], p[1], pi / 2.0, p[3] };

	// First, find the "circular" velocity
	// This already includes subKeplerian rescaling of angular momentum, and
	// partially infalling if orbit is inside ISCO
	OneIndex p_down_circ{};

	if (m_ISCOexists)
	{
		// ISCO was found; either get a purely circular velocity outside the ISCO
		// or a circular and infalling orbit inside the ISCO
		if (EquatPoint[1] >= m_ISCOr)
			p_down_circ = GetCircularVelocityd(EquatPoint);
		else
			p_down_circ = GetInsideISCOCircularVelocityd(EquatPoint);

		if (p_down_circ[0] >= 0.0)
		{
			// Finding circular (or circular-infalling) orbit failed
			if (EquatPoint[1] >= m_ISCOr)
			{
				// Try circular+infalling orbit if only circular was tried
				p_down_circ = GetInsideISCOCircularVelocityd(EquatPoint);
			}
		}
	}
	else // no ISCO exists
	{
		// Try a purely circular orbit
		p_down_circ = GetCircularVelocityd(EquatPoint);
	}
	// p_down_circ will now contain a circular (or circular-infalling) velocity
	// if the above was successful, or otherwise (still) all zeroes



	// Second, find purely radially infalling velocity
	OneIndex p_down_rad{ GetRadialVelocityd(EquatPoint) };

	// This is where we will store the final velocity with index down
	OneIndex p_down_fin{};

	if (p_down_circ[0] <= 0.0)
	{
		// We found a circular (or circular-infalling) velocity above, so the final velocity will be a mix
		// of this orbit and the purely radially infalling one

		// Metric at this point
		TwoIndex g_uu{ m_theMetric->getMetric_uu(EquatPoint) };
		TwoIndex g_dd{ m_theMetric->getMetric_dd(EquatPoint) };

		// Calculate velocities with index up
		OneIndex u_up_circ{};
		OneIndex u_up_rad{};
		for (int i = 0; i < dimension; ++i)
		{
			for (int j = 0; j < dimension; ++j)
			{
				u_up_circ[i] += g_uu[i][j] * p_down_circ[j];
				u_up_rad[j] += g_uu[i][j] * p_down_rad[j];
			}
		}

		// Prescription for final u^r velocity component using parameter beta_R
		real ur_tot{ u_up_circ[1] + (1.0 - m_betaR) * (u_up_rad[1] - u_up_circ[1]) };

		// Prescription for final Omega = u^\phi/u^t using parameter beta_Phi
		real OmegaCirc{ u_up_circ[3] / u_up_circ[0] };
		real OmegaRad{ u_up_rad[3] / u_up_rad[0] };
		real Omega_tot{ OmegaCirc + (1.0 - m_betaPhi) * (OmegaRad - OmegaCirc) };

		// Find final u^t from final u^r and final Omega's and four-velocity normalization condition
		real denom{ g_dd[0][0] + 2.0 * g_dd[0][3] * Omega_tot + g_dd[3][3] * Omega_tot * Omega_tot };
		real ut{ sqrt(fmax((-1.0 - g_dd[1][1] * ur_tot * ur_tot) / denom, 0.0)) };

		// This is the final four-velocity with index up, lower this index again now
		OneIndex u_up_tot{ ut, ur_tot, 0.0, ut * Omega_tot };
		for (int i = 0; i < dimension; ++i)
		{
			for (int j = 0; j < dimension; ++j)
			{
				p_down_fin[i] += g_dd[i][j] * u_up_tot[j];
			}
		}
	}
	else
	{
		// No circular (or circular-infalling) orbit was found, so we simply use a radially infalling orbit instead
		p_down_fin = p_down_rad;
	}

	if (std::isnan(p_down_fin[0]) || std::isnan(p_down_fin[1]) || std::isnan(p_down_fin[3]) || p_down_fin[0]>=0.0)
		p_down_fin = p_down_rad;

	return p_down_fin;
}


std::string GeneralCircularRadialFluid::getFullDescriptionStr() const
{
	// Full description string; contains ISCO radius (calculated in constructor)

	real trueISCOradius{ m_ISCOr };
	if (m_ISCOexists && m_theMetric->getrLogScale())
		trueISCOradius = exp(m_ISCOr);


	return "Circular/radial flow (sub-Keplerian parameter xi = " + std::to_string(m_subKeplerParam)
		+ ", beta_r = " + std::to_string(m_betaR) + ", beta_phi = " + std::to_string(m_betaPhi) + "; "
		+ (m_ISCOexists ? "ISCO = " + std::to_string(trueISCOradius) : "no ISCO found")
		+ ")";
}

OneIndex GeneralCircularRadialFluid::GetCircularVelocityd(const Point& p, bool subKeplerianOn) const
{
	// Returns velocity for circular (sub)Keplerian motion (outside the ISCO)
	// First the circular geodesic is calculated,
	// then if subKeplerianOn == true, the orbit is reparametrized to non-geodetic
	// subKeplerian circular motion according to the subKeplerian parameter

	// First calculate the circular geodetic motion

	TwoIndex g_uu{ m_theMetric->getMetric_uu(p) };
	ThreeIndex Christ{ m_theMetric->getChristoffel_udd(p) };

	// a,b,c carry the necessary Christoffel (indices raised) components
	// of the radial geodesic equation
	real a{};
	real b{};
	real c{};
	for (int i = 0; i < dimension; ++i)
		for (int j = 0; j < dimension; ++j)
		{
			a += g_uu[0][i] * Christ[1][i][j] * g_uu[j][0];
			b += g_uu[3][i] * Christ[1][i][j] * g_uu[j][3];
			c += 2.0 * g_uu[0][i] * Christ[1][i][j] * g_uu[j][3];
		}

	// ap, bp, cp carry the necessary metric components (indices up) for the four-velocity normalization condition equation
	real ap{ g_uu[0][0] };
	real bp{ g_uu[3][3] };
	real cp{ 2.0 * g_uu[0][3] };

	OneIndex p_down{};

	// If finding a solution fails, no circular motion found at this point
	bool solfailed{ false };

	// Solve for eta = L/E using the r geodesic equation
	real discr{ c * c - 4 * a * b };
	if (discr < 0.0)
		solfailed = true;

	if (!solfailed)
	{
		real eta{};
		if (fabs(b) > Integrators::delta_nodiv0)
			eta = 1 / (2.0 * b) * (c - sqrt(discr)); // choose root to maximize eta; note b < 0 far away
		else
		{
			if (fabs(c) > Integrators::delta_nodiv0)
				eta = a / c;
			else
			{
				solfailed = true; // both b==0 and c==0, impossible to find a solution
			}

		}

		// Now that we have eta, use the velocity normalization equation to solve for the energy E
		real denom{ ap + bp * eta * eta - cp * eta };

		if (denom >= 0.0)
			solfailed = true; // imaginary energy

		if (!solfailed)
		{
			real En_sq{ -1.0 / denom };
			p_down[0] = -sqrt(En_sq);
			p_down[3] = eta * (-p_down[0]);

			// We have now found the circular geodesic velocity.
			// If we request the subKeplerian orbit, then first rescale the angular momentum accordingly
			// and then re-calculate the energy E from the four-velocity normalization condition
			if (subKeplerianOn)
			{
				// rescale angular momentum
				p_down[3] *= m_subKeplerParam;

				if (fabs(ap) > Integrators::delta_nodiv0)
				{
					discr = cp * cp * p_down[3] * p_down[3] - 4.0 * ap * (1.0 + bp * p_down[3] * p_down[3]);
					if (discr < 0.0)
						solfailed = true;
					if (!solfailed)
						p_down[0] = 1 / (2.0 * ap) * (-cp * p_down[3] + sqrt(discr));
				}
				else
				{
					if (fabs(cp * p_down[3]) > Integrators::delta_nodiv0)
					{
						p_down[0] = -(1.0 + bp * p_down[3] * p_down[3]) / (cp * p_down[3]);
					}
					else
						solfailed = true;
				}
				if (solfailed)
				{
					ScreenOutput("Circular orbit found but subKeplerian rescaling failed; using un-rescaled circular orbit.",
						OutputLevel::Level_0_WARNING);
					p_down[3] /= m_subKeplerParam;
				}
			}
		}
	}

	return p_down; // if no solution, returns all zeros
}

OneIndex GeneralCircularRadialFluid::GetInsideISCOCircularVelocityd(const Point& p) const
{
	// Returns velocity for non-geodetic motion where E,L = (E,L)_ISCO and radially falling inward

	TwoIndex g_uu{ m_theMetric->getMetric_uu(p) };

	// Assuming g_ra cross terms vanish!
	// Radial component of velocity entirely determined by other (ISCO) velocity components and metric at this point
	real temp_sq{ -1.0 - g_uu[0][0] * m_ISCOpt * m_ISCOpt - 2.0 * g_uu[0][3] * m_ISCOpt * m_ISCOpphi - g_uu[3][3] * m_ISCOpphi * m_ISCOpphi };

	real pr{ -1.0 / sqrt(g_uu[1][1]) * sqrt(fmax(temp_sq,0.0)) };

	if (!std::isnan(pr))
		return OneIndex{ m_ISCOpt, pr, 0.0, m_ISCOpphi };
	else
		return {};
}

OneIndex GeneralCircularRadialFluid::GetRadialVelocityd(const Point& p) const
{
	// Returns velocity for pure infalling matter with E = 1 and L = 0 at infinity

	real E{ 1.0 }; // energy at infinity
	real p_t{ -E };
	real p_phi{ 0.0 }; // angular momentum at infinity = 0
	real p_theta{ 0.0 }; // equatorial trajectory

	TwoIndex g_uu{ m_theMetric->getMetric_uu(p) };

	real g_tt{ g_uu[0][0] };
	real g_tr{ g_uu[0][1] };
	real g_rr{ g_uu[1][1] };

	// Radial component entirely fixed by velocity components at infinity and metric at this point
	real p_r{ 1.0 / g_rr * (E * g_tr + sqrt(fmax(- g_rr + E * E * (g_tr * g_tr - g_tt * g_rr),0.0)) )};

	OneIndex p_down{ p_t, p_r, p_theta, p_phi };

	return p_down;
}

void GeneralCircularRadialFluid::FindISCO()
{
	// This helper function finds the ISCO.
	// We use a binary search to converge on the ISCO value; the initial outer bounds
	// for a metric with horizon are the horizon radius and 10*(horizon radius) (in true radii, not log(r) coordinates)

	real lowerbound{ 0.0 };
	real upperbound{ 1000.0 };
	const SphericalHorizonMetric* sphermetric = dynamic_cast<const SphericalHorizonMetric*>(m_theMetric);
	if (sphermetric)
	{
		lowerbound = sphermetric->getrLogScale() ? log(sphermetric->getHorizonRadius()) : sphermetric->getHorizonRadius();
		upperbound = sphermetric->getrLogScale() ? log(10.0 * sphermetric->getHorizonRadius()) : 10.0 * sphermetric->getHorizonRadius();
	}

	// Perform binary search for ISCO
	// Allow only 1000 iterations max
	real currentr{ (lowerbound + upperbound) / 2.0 };
	int iterations{ 0 };
	bool exactfound{ false }; // in case we converge exactly on the ISCO
	bool nextsmaller{ true };
	while (upperbound - lowerbound > 2.0 * Integrators::Derivative_hval && iterations < 1000 && !exactfound)
	{
		// If this returns non-zero values than it found a circular orbit
		OneIndex currentp{ GetCircularVelocityd(Point{ 0.0,currentr ,pi / 2.0,0.0 }, false) };
		if (currentp[0] < 0.0) // found pure circular orbit at this point
		{
			// Calculate stability of this circular orbit by considering a small perturbation of the radius
			// This is \partial_r(g^{ab}\Gamma^r_{bc}g^{cd})
			TwoIndex ChristrRaisedDer{ GetChristrRaisedDer(currentr) };
			// rdd is the rhs of the resulting perturbed geodesic equation for r
			real rdd{};
			for (int i = 0; i < dimension; ++i)
				for (int j = 0; j < dimension; ++j)
					rdd += -currentp[i] * ChristrRaisedDer[i][j] * currentp[j];

			if (rdd < 0.0)
				// stable orbit, so ISCO < currentr
				nextsmaller = true;
			else if (rdd > 0.0) 
				// unstable orbit, so ISCO > currentr
				nextsmaller = false;
			else
				// ISCO = currentr exactly
				exactfound = true;
		}
		else
		{
			// Did not find a circular orbit at all at this radius, so ISCO > currentr
			nextsmaller = false;
		}
		if (!exactfound && nextsmaller) // ISCO < currentr
		{
			upperbound = currentr;
			currentr = (lowerbound + upperbound) / 2.0;
		}
		else if (!exactfound && !nextsmaller) // ISCO > currentr
		{
			lowerbound = currentr;
			currentr = (lowerbound + upperbound) / 2.0;
		}

		++iterations; // update iteration count
	}

	// Store the ISCO momentum components
	OneIndex pdownISCO{ GetCircularVelocityd(Point{0.0, exactfound ? m_ISCOr : upperbound, pi / 2.0, 0.0}) };
	if (pdownISCO[0] >= 0.0)
	{
		ScreenOutput("Finding ISCO and ISCO momentum failed!", OutputLevel::Level_4_DEBUG);
		m_ISCOexists = false;
	}
	else
	{
		m_ISCOexists = true;
		m_ISCOr = exactfound ? currentr : upperbound;  // use upper bound to ensure a circular geodesic momentum was/can be found
		m_ISCOpt = pdownISCO[0];
		m_ISCOpphi = pdownISCO[3];
	}
}


TwoIndex GeneralCircularRadialFluid::GetChristrRaisedDer(real r) const
{
	// This function calculates \partial_r(g^ { ab }\Gamma ^ r_{ bc }g^ { cd })
	// We do so by calculating the central difference with O(h^4) accuracy, but taking the
	// sqrt of the usual h used for derivatives

	Point BaseP{ 0.0, r, pi / 2.0, 0.0 };
	Point ShiftP{ 0.0, sqrt(Integrators::Derivative_hval), 0.0, 0.0 };

	TwoIndex g_uuplusplus{ m_theMetric->getMetric_uu(BaseP + 2.0 * ShiftP) };
	ThreeIndex Gammaplusplus{ m_theMetric->getChristoffel_udd(BaseP + 2.0 * ShiftP) };
	TwoIndex g_uuplus{ m_theMetric->getMetric_uu(BaseP + ShiftP) };
	ThreeIndex Gammaplus{ m_theMetric->getChristoffel_udd(BaseP + ShiftP) };
	TwoIndex g_uumin{ m_theMetric->getMetric_uu(BaseP - ShiftP) };
	ThreeIndex Gammamin{ m_theMetric->getChristoffel_udd(BaseP - ShiftP) };
	TwoIndex g_uuminmin{ m_theMetric->getMetric_uu(BaseP - 2.0 * ShiftP) };
	ThreeIndex Gammaminmin{ m_theMetric->getChristoffel_udd(BaseP - 2.0 * ShiftP) };

	TwoIndex FullObjPlusPlus{};
	TwoIndex FullObjPlus{};
	TwoIndex FullObjMin{};
	TwoIndex FullObjMinMin{};
	for (int i = 0; i < dimension; ++i)
	{
		for (int j = 0; j < dimension; ++j)
		{
			for (int k = 0; k < dimension; ++k)
			{
				for (int l = 0; l < dimension; ++l)
				{
					FullObjPlusPlus[i][j] += g_uuplusplus[i][k] * Gammaplusplus[1][k][l] * g_uuplusplus[l][j];
					FullObjPlus[i][j] += g_uuplus[i][k] * Gammaplus[1][k][l] * g_uuplus[l][j];
					FullObjMin[i][j] += g_uumin[i][k] * Gammamin[1][k][l] * g_uumin[l][j];
					FullObjMinMin[i][j] += g_uuminmin[i][k] * Gammaminmin[1][k][l] * g_uuminmin[l][j];
				}
			}
		}
	}

	return (-1.0 * FullObjPlusPlus + 8.0 * FullObjPlus - 8.0 * FullObjMin + FullObjMinMin) / (12.0 * sqrt(Integrators::Derivative_hval));
}
