#ifndef _FOORT_INTEGRATORS_H
#define _FOORT_INTEGRATORS_H

#include"Geometry.h"
#include"Metric.h"


// Forward declaration needed
class Source;

using GeodesicIntegratorFunc = void (*)(Point, OneIndex, Point&, OneIndex&, real&, const Metric*, const Source*);


namespace Integrators
{
	constexpr real delta_nodiv0 = 1e-20;
	constexpr real SmallestPossibleStepsize = 1e-12;

	inline real epsilon{ 0.03 };

	void IntegrateGeodesicStep_RK4(Point curpos, OneIndex curvel,
		Point& nextpos, OneIndex& nextvel, real& stepsize, const Metric* theMetric, const Source* theSource);
}

#endif
