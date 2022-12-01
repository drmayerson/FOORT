#ifndef _FOORT_INTEGRATORS_H
#define _FOORT_INTEGRATORS_H

///////////////////////////////////////////////////////////////////////////////////////
////// INTEGRATORS.H
////// Declarations of integrator functions that integrate geodesic equation
////// All definitions in Integrators.cpp
///////////////////////////////////////////////////////////////////////////////////////

#include "Geometry.h" // for basic tensor objects
#include "Metric.h" // for the metric

// Forward declaration needed of Source
// (Source is declared in Geodesic.h, but we want to avoid a header loop!)
class Source;


// This is the structure of an function that integrates the geodesic equation one step
// It takes as arguments:
// - current position, current velocity
// - references to: next position, next velocity, affine parameter step size (which the functions sets)
// - pointers to: the Metric, the Source that are used to evaluate the geodesic equation
using GeodesicIntegratorFunc = void (*)(Point, OneIndex, Point&, OneIndex&, real&, const Metric*, const Source*);

// Namespace for integrator constants and functions
namespace Integrators
{
	// This is used to avoid dividing by zero
	constexpr real delta_nodiv0 = 1e-20;
	// The affine parameter must always go forward by at least this amount
	constexpr real SmallestPossibleStepsize = 1e-12;

	// This is the base step size to be taken (the integrator will adapt this if necessary)
	inline real epsilon{ 0.03 };

	// This is a GeodesicIntegratorFunc
	// Using the Runge-Kutta-4 algorithm to integrate the geodesic equation
	void IntegrateGeodesicStep_RK4(Point curpos, OneIndex curvel,
		Point& nextpos, OneIndex& nextvel, real& stepsize, const Metric* theMetric, const Source* theSource);
}

#endif
