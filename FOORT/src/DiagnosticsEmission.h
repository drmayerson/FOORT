#ifndef _FOORT_DIAGNOSTICS_EMISSION_H
#define _FOORT_DIAGNOSTICS_EMISSION_H

#include "Geometry.h"	 // Tensor objects
#include "Metric.h"		 // for Metric functions
#include "InputOutput.h" // for ScreenOutput

#include <string> // for strings
#include <cmath>  // for fmax, fmin

/**
 * @file DiagnosticsEmission.h
 * @author Daniel R. Mayerson
 * @brief Declarations of emission and fluid velocity models used for equatorial disc emission.
 * @version 1.0
 * @date 2024-12-16
 * @copyright Copyright (c) 2024
 */

/**
 * @brief Emmision model abstract base class
 *
 */
struct EmissionModel
{
public:
	// Virtual destructor to ensure correct destruction
	virtual ~EmissionModel() = default;

	// Function which returns the emitted brightness intensity at Point p
	// Note: always pass the true radius (not log(r)) to EmissionModel!
	virtual real GetEmission(const Point &p) const = 0;

	// Description string getter
	virtual std::string getFullDescriptionStr() const;
};

/**
 * @brief The Johnson SU emission model used in GLM 2020, PRD 102(12):124004
 */
struct GLMJohnsonSUEmission final : public EmissionModel
{
public:
	// Constructor
	GLMJohnsonSUEmission(real mu, real gamma, real sigma) : m_mu{mu}, m_gamma{gamma}, m_sigma{sigma}
	{
	}

	// Emitted brightness (only depends on radius)
	real GetEmission(const Point &p) const final;

	// Description string getter
	std::string getFullDescriptionStr() const final;

private:
	//! mu parameter
	const real m_mu;
	//! gamma parameter
	const real m_gamma;
	//! sigma parameter
	const real m_sigma;
};

///////////////

/**
 * @brief Fluid velocity model abstract base class
 *
 */
struct FluidVelocityModel
{
public:
	// Constructor is passed Metric pointer
	FluidVelocityModel(const Metric *const theMetric) : m_theMetric{theMetric} {}

	// Virtual destructor to ensure correct destruction
	virtual ~FluidVelocityModel() = default;

	// Get the local four-velocity (with index down!) of the fluid at Point p
	virtual OneIndex GetFourVelocityd(const Point &p) const = 0;

	// Description string getter
	virtual std::string getFullDescriptionStr() const;

protected:
	// Metric pointer, used for e.g. calculating geodesic orbits
	const Metric *const m_theMetric;
};

/**
 * @brief General circular radial fluid velocity model
 * @details This fluid velocity model has three tuneable parameters and represents fluid travelling at a mix of
 * (sub)Keplerian circular orbits and radially infalling orbits in the equatorial plane. Details can be found in e.g. https://doi.org/10.1103/PhysRevD.107.043030 .
 */
struct GeneralCircularRadialFluid final : public FluidVelocityModel
{
	// Constructor with three parameters and Metric pointer (which is passed to base class constructor)
	GeneralCircularRadialFluid(real subKeplerParam, real betar, real betaphi, const Metric *const theMetric,
							   real ISCO_lowerbound, real ISCO_upperbound) : m_subKeplerParam{fmin(fmax(subKeplerParam, 0.0), 1.0)}, m_betaR{fmin(fmax(betar, 0.0), 1.0)},
																			 m_betaPhi{fmin(fmax(betaphi, 0.0), 1.0)}, FluidVelocityModel(theMetric),
																			 m_ISCOlowerbound{ISCO_lowerbound}, m_ISCOupperbound{ISCO_upperbound}
	{
		// Do some checks on three params, which must lie between 0.0 and 1.0 (note that they are adjusted as such in
		// initializer above)
		if (subKeplerParam < 0.0)
			ScreenOutput("Sub-Keplerian parameter must be between 0 and 1; adjusting to 0", OutputLevel::Level_0_WARNING);
		if (subKeplerParam > 1.0)
			ScreenOutput("Sub-Keplerian parameter must be between 0 and 1; adjusting to 1", OutputLevel::Level_0_WARNING);

		if (betar < 0.0)
			ScreenOutput("beta_r parameter must be between 0 and 1; adjusting to 0", OutputLevel::Level_0_WARNING);
		if (betar > 1.0)
			ScreenOutput("beta_r parameter must be between 0 and 1; adjusting to 1", OutputLevel::Level_0_WARNING);

		if (betaphi < 0.0)
			ScreenOutput("beta_phi parameter must be between 0 and 1; adjusting to 0", OutputLevel::Level_0_WARNING);
		if (betaphi > 1.0)
			ScreenOutput("beta_phi parameter must be between 0 and 1; adjusting to 1", OutputLevel::Level_0_WARNING);

		// Find the (equatorial) ISCO for this Metric
		FindISCO();
	}

	// Get the local four-velocity of the fluid according to this model
	// Note: will always calculate with Point p exactly on the equator theta=pi/2, despite what p[2] may be passed
	OneIndex GetFourVelocityd(const Point &p) const final;

	// Description string getter
	std::string getFullDescriptionStr() const final;

private:
	// Three parameters determining the flow
	//! Sub-Keplerian parameter (0 = Keplerian, 1 = sub-Keplerian)
	const real m_subKeplerParam;
	//! Radial velocity parameter
	const real m_betaR;
	//! Azimuthal velocity parameter
	const real m_betaPhi;

	// Helper function to get circular (sub)Keplerian velocity outside the ISCO
	OneIndex GetCircularVelocityd(const Point &p, bool subKeplerianOn = true) const;
	// Helper function to get circular and infalling (sub)Keplerian velocity inside ISCO
	// (according to prescription of Cunningham)
	OneIndex GetInsideISCOCircularVelocityd(const Point &p) const;

	// Helper function to get radial infalling velocity
	OneIndex GetRadialVelocityd(const Point &p) const;

	// Helper function to find the ISCO (called in Constructor)
	void FindISCO();
	//! Flag to indicate if ISCO exists for this Metric
	bool m_ISCOexists{false};
	//! ISCO radius
	real m_ISCOr{-1.0};
	//! Lower bound for ISCO radius search
	real m_ISCOlowerbound;
	//! Upper bound for ISCO radius search
	real m_ISCOupperbound;
	//! ISCO t momentum
	real m_ISCOpt{};
	//! ISCO phi momentum
	real m_ISCOpphi{};

	// Helper function which returns \partial_r(g^{ab}\Gamma^r_{bc}g^{cd}),
	// whose sign (after contracted with p_a p_d of the corresponding circular orbit)
	// tells us if the circular orbit considered is stable or not
	TwoIndex GetChristrRaisedDer(real r) const;
};

#endif
