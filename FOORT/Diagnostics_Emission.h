#ifndef _FOORT_DIAGNOSTICS_EMISSION_H
#define _FOORT_DIAGNOSTICS_EMISSION_H

#include "Geometry.h" // Tensor objects
#include "Metric.h" // for Metric functions
#include "InputOutput.h" // for ScreenOutput

#include <string> // for strings
#include <cmath> // for fmax, fmin

// Here we declare the emission and fluid velocity models used for equatorial disc emission

// Emission model abstract base class
struct EmissionModel
{
public:
	// Virtual destructor to ensure correct destruction
	virtual ~EmissionModel() = default;

	// Function which returns the emitted brightness intensity at Point p
	// Note: always pass the true radius (not log(r)) to EmissionModel!
	virtual real GetEmission(const Point& p) const = 0;

	// Description string getter
	virtual std::string getFullDescriptionStr() const;
};

// The Johnson SU emission model used in GLM
struct GLMJohnsonSUEmission final : public EmissionModel
{
public:
	// Constructor
	GLMJohnsonSUEmission(real mu, real gamma, real sigma) : m_mu{ mu }, m_gamma{ gamma }, m_sigma{ sigma }
	{}

	// Emitted brightness (only depends on radius)
	real GetEmission(const Point& p) const final;

	// Description string getter
	std::string getFullDescriptionStr() const final;

private:
	// mu, gamma, sigma are the three parameters of the model
	const real m_mu;
	const real m_gamma;
	const real m_sigma;
};

///////////////

// Fluid velocity abstract base class
struct FluidVelocityModel
{
public:
	// Constructor is passed Metric pointer
	FluidVelocityModel(const Metric* const theMetric) : m_theMetric{ theMetric } {}

	// Virtual destructor to ensure correct destruction
	virtual ~FluidVelocityModel() = default;

	// Get the local four-velocity (with index down!) of the fluid at Point p
	virtual OneIndex GetFourVelocityd(const Point& p) const = 0;

	// Description string getter
	virtual std::string getFullDescriptionStr() const;

protected:
	// Metric pointer, used for e.g. calculating geodesic orbits
	const Metric* const m_theMetric;
};

// This fluid velocity model has three tuneable parameters and represents fluid travelling at a mix of
// (sub)Keplerian circular orbits and radially infalling orbits in the equatorial plane
struct GeneralCircularRadialFluid final : public FluidVelocityModel
{
	// Constructor with three parameters and Metric pointer (which is passed to base class constructor)
	GeneralCircularRadialFluid(real subKeplerParam, real betar, real betaphi, const Metric* const theMetric) :
		m_subKeplerParam{ fmin(fmax(subKeplerParam,0.0),1.0) }, m_betaR{ fmin(fmax(betar,0.0),1.0) },
		m_betaPhi{ fmin(fmax(betaphi,0.0),1.0) }, FluidVelocityModel(theMetric)
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
	OneIndex GetFourVelocityd(const Point& p) const final;

	// Description string getter
	std::string getFullDescriptionStr() const final;

private:
	// Three parameters determining the flow
	const real m_subKeplerParam;
	const real m_betaR;
	const real m_betaPhi;

	// Helper function to get circular (sub)Keplerian velocity outside the ISCO
	OneIndex GetCircularVelocityd(const Point& p, bool subKeplerianOn = true) const;
	// Helper function to get circular and infalling (sub)Keplerian velocity inside ISCO
	// (according to prescription of Cunningham)
	OneIndex GetInsideISCOCircularVelocityd(const Point& p) const;

	// Helper function to get radial infalling velocity
	OneIndex GetRadialVelocityd(const Point& p) const;

	// Helper function to find the ISCO (called in Constructor)
	void FindISCO();
	// ISCO radius and momentum (index down) components
	bool m_ISCOexists{ false };
	real m_ISCOr{ -1.0 };
	real m_ISCOpt{};
	real m_ISCOpphi{};

	// Helper function which returns \partial_r(g^{ab}\Gamma^r_{bc}g^{cd}),
	// whose sign (after contracted with p_a p_d of the corresponding circular orbit)
	// tells us if the circular orbit considered is stable or not
	TwoIndex GetChristrRaisedDer(real r) const;
};

#endif