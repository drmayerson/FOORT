#ifndef _FOORT_METRICS_ALL_BENCH_H
#define _FOORT_METRICS_ALL_BENCH_H

#include"Geometry.h"

#include<cassert>
#include<functional>
#include<variant>

// Termination conditions; can be set by the Metric OR by Terminations
enum class Term
{
	term_Continue = 0,		// All is right, continue integrating geodesic
	term_Horizon,			// STOP, encountered horizon (set by Metric)
	term_Singularity,		// STOP, encountered singularity/center (set by Metric)
	term_BoundarySphere,	// STOP, encountered boundary sphere (set by Termination::BoundarySphereTermination)
	term_TimeOut,			// STOP, taken too many steps (set by Termination::TimeOutTermination)

	term_Maxterms			// Number of termination conditions that exist
};

///
// INHERITANCE BASED CLASSES
///

// The abstract base class for all Metrics.
class Metric
{
public:
	virtual ~Metric() = default;
	// Get the metric, indices down
	virtual TwoIndex getMetric_dd(const Point& p) const = 0;
};

// The type of metric that has a spherical horizon
class SphericalHorizonMetric : public Metric
{
protected:
	// Radius of the horizon
	real m_HorizonRadius;
public:
	// Constructor that initializes radius and distance to horizon necessary for termination
	SphericalHorizonMetric(real HorizonRadius);
};

// The Kerr metric
class KerrMetricINHERITANCE final : public SphericalHorizonMetric
{
private:
	real m_aParam;

public:

	// No constructor without passing parameters, and no copies allowed
	KerrMetricINHERITANCE() = delete;
	KerrMetricINHERITANCE(const KerrMetricINHERITANCE& other) = delete;
	KerrMetricINHERITANCE& operator=(const KerrMetricINHERITANCE& other) = delete;

	// Constructor setting paramater a
	KerrMetricINHERITANCE(real aParam = 0.0);

	TwoIndex getMetric_dd(const Point& p) const final;
};

class PlaceHolderMetricINHERITANCE final : public SphericalHorizonMetric
{
public:
	PlaceHolderMetricINHERITANCE() : SphericalHorizonMetric(0.0) {};
	TwoIndex getMetric_dd(const Point& p) const final;
};


///
// VARIANT BASED CLASSES
///


struct KerrMetricVARIANT
{
	KerrMetricVARIANT(real thea) : aParam(thea), HorizonRadius(1 + sqrt(1 - aParam * aParam)) {}

	const real aParam;
	const real HorizonRadius;
	real AtHorizonEps{ 0.01 };
};

struct PlaceHolderMetricVARIANT
{
	real AtHorizonEps;
};

//using MetricVariantObj = std::variant<KerrMetricVARIANT, PlaceHolderMetricVARIANT>;
using MetricVariantObj = std::variant<PlaceHolderMetricVARIANT, KerrMetricVARIANT>;

// Get the metric, indices down
struct getMetric_dd_VISITOR
{
	Point p;

	TwoIndex operator()(const KerrMetricVARIANT& TheMetric);
	TwoIndex operator()(const PlaceHolderMetricVARIANT& TheMetric);
};

struct getCorrectMetricFunctionVARIANT
{

	std::function<TwoIndex(Point)> operator()(const KerrMetricVARIANT& TheMetric) const;
	std::function<TwoIndex(Point)> operator()(const PlaceHolderMetricVARIANT& TheMetric) const;
};


struct getMetric_dd_IMPROVED_VARIANT
{
	TwoIndex operator()(const KerrMetricVARIANT& TheMetric, Point p) const;
	TwoIndex operator()(const PlaceHolderMetricVARIANT& TheMetric, Point p) const;
};

TwoIndex getMetricSTDGET(const MetricVariantObj& theVarMetric, Point p);

///
// DIRECT CALL FUNCTION
///

TwoIndex MetricDIRECTCALL(const Point& p, real m_aParam);


#endif
