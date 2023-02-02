#include "Integrators.h" // We are defining functions declared here

#include "Geodesic.h" // Needed for Source member functions

#include <algorithm> // for std::min, std::max
#include <cmath> // for std::abs

#include <sstream> // std::stringstream
#include <iostream> // std::scientific 

std::string Integrators::GetFullIntegratorDescription()
{
	// Helper function to convert (small) doubles to string in scientific notation
	auto to_string_scientific = [](real num) -> std::string
	{
		std::stringstream tempstream;
		tempstream << std::scientific << num;
		return tempstream.str();
	};

	// Return full descriptive string of integrator and all integrator options
	std::string fullintegratorstring{ "Integrator: " };
	fullintegratorstring += Integrators::IntegratorDescription;
	if (Integrators::IntegratorDescription == "Verlet")
	{
		fullintegratorstring += " (velocity tolerance: " + to_string_scientific(Integrators::VerletVelocityTolerance) + ")";
	}
	return fullintegratorstring + ", basic step size: " + to_string_scientific(Integrators::epsilon)
		+ ", min. step size: " + to_string_scientific(Integrators::SmallestPossibleStepsize)
		+ ", derivative h: " + to_string_scientific(Integrators::Derivative_hval);
}


real Integrators::GetAdaptiveStep(Point curpos, OneIndex curvel)
{
	//// Determine the (affine parameter) step size to take
	// algorithm as in Raptor (eqs (21)-(24)), which is taken from Noble et al. (2007) & Dolence et al. (2009)
	real dlambda_x1 = epsilon / (std::abs(curvel[1]) + delta_nodiv0);
	real dlambda_x2 = epsilon * std::min(curpos[2], pi - curpos[2]) / (std::abs(curvel[2]) + delta_nodiv0);
	real dlambda_x3 = epsilon / (std::abs(curvel[3] + delta_nodiv0));

	real h = 1 / (1 / std::abs(dlambda_x1) + 1 / std::abs(dlambda_x2) + 1 / std::abs(dlambda_x3));
	// Make sure we take at least the smallest allowed step size
	h = std::max(h, SmallestPossibleStepsize);

	return h;
}

// This is a GeodesicIntegratorFunc
// Integrate the geodesic equation by one step using Runge-Kutta-4
void Integrators::IntegrateGeodesicStep_RK4(Point curpos, OneIndex curvel,
	Point& nextpos, OneIndex& nextvel, real& stepsize, const Metric* theMetric, const Source* theSource)
{
	real h = GetAdaptiveStep(curpos, curvel);

	//// Construct geodesic equation
	// The rhs of the geodesic equation for the velocity is:
	// d/d\lambda(u^a) = - Gamma^a_{bc} u^b u^c + [Source(x,u)]^a;
	// This helper function computes the rhs of the geodesic equation
	auto geoRHS = [theMetric, theSource](Point p, OneIndex v)->OneIndex
	{
		ThreeIndex christ{ theMetric->getChristoffel_udd(p) };
		OneIndex ret{ theSource->getSource(p,v) };
		for (int i = 0; i < dimension; ++i)
			for (int j = 0; j < dimension; ++j)
				for (int k = 0; k < dimension; ++k)
					ret[i] -= christ[i][j][k] * v[j] * v[k];
		return ret;
	};


	//// Perform Runge-Kutta 4 algorithm

	// RK step 1
	OneIndex k1{ geoRHS(curpos,curvel) };
	Point l1{ curvel };

	// RK step 2
	OneIndex k2{ geoRHS(curpos + 0.5 * h * l1,curvel + 0.5 * h * k1) };
	Point l2{ curvel + 0.5 * h * k1 };

	// RK step 3
	OneIndex k3{ geoRHS(curpos + 0.5 * h * l2,curvel + 0.5 * h * k2) };
	Point l3{ curvel + 0.5 * h * k2 };

	// RK step 4
	OneIndex k4{ geoRHS(curpos +  h * l3,curvel +  h * k3) };
	Point l4{ curvel + h * k3 };

	// RK totals give new step
	nextvel = curvel + h / 6.0 * (k1 + 2 * k2 + 2 * k3 + k4);
	nextpos = curpos + h / 6.0 * (l1 + 2 * l2 + 2 * l3 + l4);
	stepsize = h;
}


// This is a GeodesicIntegratorFunc
// Integrate the geodesic equation by one step using velocity Verlet algorithm
void Integrators::IntegrateGeodesicStep_Verlet(Point curpos, OneIndex curvel,
	Point& nextpos, OneIndex& nextvel, real& stepsize, const Metric* theMetric, const Source* theSource)
{
	real h = GetAdaptiveStep(curpos, curvel);

	//// Construct geodesic equation
	// The rhs of the geodesic equation for the velocity is:
	// d/d\lambda(u^a) = - Gamma^a_{bc} u^b u^c + [Source(x,u)]^a;
	// This helper function computes the rhs of the geodesic equation
	auto geoRHS = [theMetric, theSource](Point p, OneIndex v)->OneIndex
	{
		ThreeIndex christ{ theMetric->getChristoffel_udd(p) };
		OneIndex ret{ theSource->getSource(p,v) };
		for (int i = 0; i < dimension; ++i)
			for (int j = 0; j < dimension; ++j)
				for (int k = 0; k < dimension; ++k)
					ret[i] -= christ[i][j][k] * v[j] * v[k];
		return ret;
	};

	// Cartesian norm (squared) of vector helper function
	auto cartvecsq = [](OneIndex v1)->real
	{
		double ret{ 0 };
		for (real r : v1)
		{
			ret += r * r;
		}
		return ret;
	};

	//// Perform velocity Verlet algorithm (see Dolence et al. (2009) eq. (14))
	OneIndex accelcur{ geoRHS(curpos,curvel) };

	nextpos = curpos + h * curvel + h * h / 2.0 * accelcur;

	OneIndex velintermed{ curvel + h * accelcur };

	OneIndex accelstep{ geoRHS(nextpos,velintermed) };

	nextvel = curvel + h / 2.0 * (accelcur + accelstep);

	// Check fractional error, if above tolerance than iterate velocity again
	while (VerletVelocityTolerance > 0.0 
		&& cartvecsq(nextvel - velintermed) / cartvecsq(nextvel) > VerletVelocityTolerance * VerletVelocityTolerance)
	{
		velintermed = nextvel;
		accelstep = geoRHS(nextpos, velintermed);
		nextvel = curvel + h / 2.0 * (accelcur + accelstep);
	}

	// nextpos & nexvel are set; only need to still set stepsize and the new step is finished
	stepsize = h;
}

