#include"Metric_VARIANT.h"

TwoIndex getMetric_dd::operator()(const KerrMetric& TheMetric)
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

TwoIndex getMetric_dd::operator()(const PlaceHolderMetric& TheMetric)
{
	return { TheMetric.AtHorizonEps };
}

std::function<TwoIndex(Point)> getCorrectMetricFunction::operator()(const KerrMetric& TheMetric) const
{
	return [TheMetric](Point p)->TwoIndex {return NEWgetMetric_dd{}(TheMetric, p); };
}

std::function<TwoIndex(Point)> getCorrectMetricFunction::operator()(const PlaceHolderMetric& TheMetric) const
{
	return [TheMetric](Point p)->TwoIndex {return NEWgetMetric_dd{}(TheMetric, p); };
}

TwoIndex NEWgetMetric_dd::operator()(const KerrMetric& TheMetric, Point p) const
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

TwoIndex NEWgetMetric_dd::operator()(const PlaceHolderMetric& TheMetric, Point p) const
{
	assert(false);
	if (TheMetric.AtHorizonEps == 0.0&&p[0]==0.0)
		return {};
	return {};
}



//TwoIndexFuncPointer getCorrectMetricFunctionPT::operator()(const KerrMetric& TheMetric)
//{
//	return  &[TheMetric](Point p)->TwoIndex {return NEWgetMetric_dd{}(TheMetric, p); };
//}
//
//TwoIndexFuncPointer getCorrectMetricFunctionPT::operator()(const PlaceHolderMetric& TheMetric)
//{
//	return & [TheMetric](Point p)->TwoIndex {return NEWgetMetric_dd{}(TheMetric, p); };
//}