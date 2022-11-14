#ifndef _FOORT_METRIC_VARIANT_H
#define _FOORT_METRIC_VARIANT_H

#include"Geometry.h"

#include<cassert>

#include<functional>

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

struct KerrMetric
{
	KerrMetric(real thea) : aParam(thea), HorizonRadius(1 + sqrt(1 - aParam * aParam)) {}

	const real aParam;
	const real HorizonRadius;
	real AtHorizonEps{0.01};
};

struct PlaceHolderMetric
{
	real AtHorizonEps;
};

// Get the metric, indices down
struct getMetric_dd
{
	Point p;

	TwoIndex operator()(const KerrMetric& TheMetric);
	TwoIndex operator()(const PlaceHolderMetric& TheMetric);
};

// Get the metric, indices up
struct getMetric_uu
{
	Point p;

	TwoIndex operator()(const KerrMetric& TheMetric);
	
};


// Check if there is an internal termination condition satisfied (horizon, singularity),
// otherwise returns term_Continue if all is well
struct InternalTerminate
{
	Point p;

	Term operator()(const KerrMetric& TheMetric);
};

// Get the Christoffel symbol, indices up-down-down
template<typename AnyMetric>
struct getChristoffel_udd
{
	Point p;

	ThreeIndex operator()(const AnyMetric& TheMetric);
};

// Get the Riemann tensor, indices up-down-down-down
// FourIndex getRiemann_uddd(const Point& p) const;
// Get the Kretschmann scalar
// real getKretschmann(const Point& p) const;


// Metric SelectMetric(configobject cfg);


struct getCorrectMetricFunction
{

	std::function<TwoIndex(Point)> operator()(const KerrMetric& TheMetric) const;
	std::function<TwoIndex(Point)> operator()(const PlaceHolderMetric& TheMetric) const;
};


struct NEWgetMetric_dd
{
	TwoIndex operator()(const KerrMetric& TheMetric, Point p) const;
	TwoIndex operator()(const PlaceHolderMetric& TheMetric, Point p) const;
};

using TwoIndexFuncPointer = TwoIndex(*)(Point);



//typedef TwoIndex(*TwoIndexFuncPointer)(Point);
//
//struct getCorrectMetricFunctionPT
//{
//
//	TwoIndexFuncPointer operator()(const KerrMetric& TheMetric);
//	TwoIndexFuncPointer operator()(const PlaceHolderMetric& TheMetric);
//};



#endif

