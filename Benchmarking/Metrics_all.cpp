#include"Geometry.h"
#include"Metrics_all.h"

#include<variant>

///
// INHERITANCE BASED CLASSES
///


SphericalHorizonMetric::SphericalHorizonMetric(real HorizonRadius)
	: m_HorizonRadius{ HorizonRadius }
{}

KerrMetricINHERITANCE::KerrMetricINHERITANCE(real aParam)
	: m_aParam{ aParam }, SphericalHorizonMetric(1 + sqrt(1 - m_aParam * m_aParam))
{
	assert(dimension == 4 && "Cannot construct Kerr metric in spacetime dimension other than 4!");
}

TwoIndex KerrMetricINHERITANCE::getMetric_dd(const Point& p) const
{
	// ScreenOutput("Kerr metric dd at " + toString(p));

	real r = p[1];
	real theta = p[2];
	real sint = sin(theta);
	real cost = cos(theta);
	real sigma = r * r + m_aParam * m_aParam * cost * cost;
	real delta = r * r + m_aParam * m_aParam - 2. * r;
	real A_ = (r * r + m_aParam * m_aParam) * (r * r + m_aParam * m_aParam)
		- delta * m_aParam * m_aParam * sint * sint;

	// Covariant metric elements
	real g00 = -(1. - 2. * r / sigma);
	real g11 = sigma / delta;
	real g22 = sigma;
	real g33 = A_ / sigma * sint * sint;
	real g03 = -2. * m_aParam * r * sint * sint / sigma;

	return TwoIndex{ {{g00, 0,0, g03 }, {0,g11,0,0}, {0,0,g22,0},{g03,0,0,g33}} };
}



///
// VARIANT BASED CLASSES
///

TwoIndex getMetric_dd_VISITOR::operator()(const KerrMetricVARIANT& TheMetric)
{
	// ScreenOutput("New Kerr metric dd at " + toString(p));

	real a = TheMetric.aParam;
	real r = p[1];
	real theta = p[2];
	real sint = sin(theta);
	real cost = cos(theta);
	real sigma = r * r + a * a * cost * cost;
	real delta = r * r + a * a - 2. * r;
	real A_ = (r * r + a * a) * (r * r + a * a)
		- delta * a * a * sint * sint;

	// Covariant metric elements
	real g00 = -(1. - 2. * r / sigma);
	real g11 = sigma / delta;
	real g22 = sigma;
	real g33 = A_ / sigma * sint * sint;
	real g03 = -2. * a * r * sint * sint / sigma;

	return TwoIndex{ {{g00, 0,0, g03 }, {0,g11,0,0}, {0,0,g22,0},{g03,0,0,g33}} };
}

TwoIndex getMetric_dd_VISITOR::operator()(const PlaceHolderMetricVARIANT& TheMetric)
{
	return { TheMetric.AtHorizonEps };
}


std::function<TwoIndex(Point)> getCorrectMetricFunctionVARIANT::operator()(const KerrMetricVARIANT& TheMetric) const
{
	return [TheMetric](Point p)->TwoIndex {return getMetric_dd_IMPROVED_VARIANT{}(TheMetric, p); };
}

std::function<TwoIndex(Point)> getCorrectMetricFunctionVARIANT::operator()(const PlaceHolderMetricVARIANT& TheMetric) const
{
	return [TheMetric](Point p)->TwoIndex {return getMetric_dd_IMPROVED_VARIANT{}(TheMetric, p); };
}

TwoIndex getMetric_dd_IMPROVED_VARIANT::operator()(const KerrMetricVARIANT& TheMetric, Point p) const
{
	real a = TheMetric.aParam;
	real r = p[1];
	real theta = p[2];
	real sint = sin(theta);
	real cost = cos(theta);
	real sigma = r * r + a * a * cost * cost;
	real delta = r * r + a * a - 2. * r;
	real A_ = (r * r + a * a) * (r * r + a * a)
		- delta * a * a * sint * sint;

	// Covariant metric elements
	real g00 = -(1. - 2. * r / sigma);
	real g11 = sigma / delta;
	real g22 = sigma;
	real g33 = A_ / sigma * sint * sint;
	real g03 = -2. * a * r * sint * sint / sigma;

	return TwoIndex{ {{g00, 0,0, g03 }, {0,g11,0,0}, {0,0,g22,0},{g03,0,0,g33}} };
}

TwoIndex getMetric_dd_IMPROVED_VARIANT::operator()(const PlaceHolderMetricVARIANT& TheMetric, Point p) const
{
	assert(false);
	if (TheMetric.AtHorizonEps == 0.0 && p[0] == 0.0)
		return {};
	return {};
}

TwoIndex getMetricSTDGET(const MetricVariantObj& theVarMetric, Point p)
{
	return getMetric_dd_IMPROVED_VARIANT{}(std::get<0>(theVarMetric), p);
}


///
// DIRECT CALL FUNCTION
///

TwoIndex MetricDIRECTCALL(const Point& p, real m_aParam)
{
	// ScreenOutput("Kerr metric dd at " + toString(p));

	real r = p[1];
	real theta = p[2];
	real sint = sin(theta);
	real cost = cos(theta);
	real sigma = r * r + m_aParam * m_aParam * cost * cost;
	real delta = r * r + m_aParam * m_aParam - 2. * r;
	real A_ = (r * r + m_aParam * m_aParam) * (r * r + m_aParam * m_aParam)
		- delta * m_aParam * m_aParam * sint * sint;

	// Covariant metric elements
	real g00 = -(1. - 2. * r / sigma);
	real g11 = sigma / delta;
	real g22 = sigma;
	real g33 = A_ / sigma * sint * sint;
	real g03 = -2. * m_aParam * r * sint * sint / sigma;

	return TwoIndex{ {{g00, 0,0, g03 }, {0,g11,0,0}, {0,0,g22,0},{g03,0,0,g33}} };
}

