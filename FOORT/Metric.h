#ifndef _FOORT_METRIC_H
#define _FOORT_METRIC_H

#include<vector>

#include"Geometry.h"
#include"Metric.h"
#include"InputOutput.h"

// Termination conditions; can be set by the Metric OR by Terminations
enum class Term
{
	Continue = 0,		// All is right, continue integrating geodesic
	Horizon,			// STOP, encountered horizon (set by Metric)
	Singularity,		// STOP, encountered singularity/center (set by Metric)
	BoundarySphere,		// STOP, encountered boundary sphere (set by Termination::BoundarySphereTermination)
	TimeOut,			// STOP, taken too many steps (set by Termination::TimeOutTermination)

	Maxterms			// Number of termination conditions that exist
};


// The abstract base class for all Metrics.
class Metric
{
public:
	virtual ~Metric() = default;
	// Get the metric, indices down
	virtual TwoIndex getMetric_dd(const Point& p) const = 0;	
	// Get the metric, indices up
	virtual TwoIndex getMetric_uu(const Point& p) const = 0;	

	// Check if there is an internal termination condition satisfied (horizon, singularity),
	// otherwise returns term_Continue if all is well
	virtual Term InternalTerminate(const Point& p) const = 0;

	// These return the Christoffel and other derivative quantities of the metric. They are computed for any
	// metric, BUT are left as virtual functions to allow for other metrics to implement their own (more efficient)
	// way of calculating them, if so desired.
	// 
	// Get the Christoffel symbol, indices up-down-down
	virtual ThreeIndex getChristoffel_udd(const Point& p) const;
	// Get the Riemann tensor, indices up-down-down-down
	virtual FourIndex getRiemann_uddd(const Point& p) const;
	// Get the Kretschmann scalar
	virtual real getKretschmann(const Point& p) const;
protected:
	std::vector<int> m_Symmetries{};
};


// The type of metric that has a spherical horizon
class SphericalHorizonMetric : public Metric
{
protected:
	// Radius of the horizon
	const real m_HorizonRadius;
	// Relative distance to the horizon for termination
	const real m_AtHorizonEps;
	// Are we using a logarithmic r coordinate?
	const bool m_rLogScale;
public:
	// No default construction allowed, must specify horizon radius
	SphericalHorizonMetric() = delete;
	// Constructor that initializes radius and distance to horizon necessary for termination
	SphericalHorizonMetric(real HorizonRadius, real AtHorizonEps, bool rLogScale);

	// Is p at the horizon?
	Term InternalTerminate(const Point& p) const override;
};



// The Kerr metric
class KerrMetric final : public SphericalHorizonMetric
{
private:
	const real m_aParam;

public:

	// No constructor without passing parameters, and no copies allowed
	KerrMetric() = delete;
	KerrMetric(const KerrMetric& other) = delete;
	KerrMetric& operator=(const KerrMetric& other) = delete;

	// Constructor setting paramater a
	KerrMetric(real aParam=0.0, real AtHorizonEps=0.01, bool rLogScale=false);

	TwoIndex getMetric_dd(const Point& p) const final;
	TwoIndex getMetric_uu(const Point& p) const final;
};

// Flat space in spherical coordinates (4D)
class FlatSpaceMetric final : public Metric
{
public:
	// Simple (default) constructor is all that is needed
	FlatSpaceMetric();

	TwoIndex getMetric_dd(const Point& p) const final;
	TwoIndex getMetric_uu(const Point& p) const final;

	Term InternalTerminate(const Point& p) const final;
};



#endif
