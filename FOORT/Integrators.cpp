#include "Integrators.h" // We are defining functions declared here

#include "Geodesic.h" // Needed for Source member functions

#include <algorithm> // for std::min, std::max
#include <cmath> // for std::abs

// This is a GeodesicIntegratorFunc
// Integrate the geodesic equation by one step using Runge-Kutta-4
void Integrators::IntegrateGeodesicStep_RK4(Point curpos, OneIndex curvel,
	Point& nextpos, OneIndex& nextvel, real& stepsize, const Metric* theMetric, const Source* theSource)
{
	//// Determine the (affine parameter) step size to take
	// algorithm as in Raptor (eqs (21)-(24)), which is taken from Noble et al. (2007) & Dolence et al. (2009)
	real dlambda_x1 = epsilon / (std::abs(curvel[1]) + delta_nodiv0);
	real dlambda_x2 = epsilon * std::min(curpos[2], pi - curpos[2]) / (std::abs(curvel[2]) + delta_nodiv0);
	real dlambda_x3 = epsilon / (std::abs(curvel[3] + delta_nodiv0));

	real h = 1 / (1 / std::abs(dlambda_x1) + 1 / std::abs(dlambda_x2) + 1 / std::abs(dlambda_x3));
	// Make sure we take at least the smallest allowed step size
	h = std::max(h, SmallestPossibleStepsize);

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

