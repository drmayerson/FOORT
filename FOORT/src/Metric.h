#ifndef _FOORT_METRIC_H
#define _FOORT_METRIC_H

#include "Geometry.h" // Needed for basic tensor objects etc.

#include "Spline.h"		  // needed for spline interpolation, for the Boson star metric
#include "Grid.h"		  // needed for the grid class, for the Rotating Boson star metric
#include "Interpolator.h" // needed for the Interpolator class, for the Rotating Boson star metric
#include <string>		  // for strings
#include <vector>		  // needed for the (non-fixed size) vector of symmetries in the metric

/**
 * @file Metric.h
 * @author Daniel R. Mayerson
 * @brief Declarations of abstract base Metric class and all its descendants.
 * @version 0.1
 * @date 2025-01-14
 * @copyright Copyright (c) 2025
 */

/**
 * @brief The abstract base class for all Metrics.
 *
 */
class Metric
{
public:
	// Virtual destructor to ensure correct destruction of descendants
	virtual ~Metric() = default;

	Metric(bool rlogscale = false);

	// Basic functions that return the metric with indices down or up:
	// pure virtual as they must be defined in the descendant classes.
	//
	// Get the metric at Point p, indices down
	virtual TwoIndex getMetric_dd(const Point &p) const = 0;
	// Get the metric at Point p, indices up
	virtual TwoIndex getMetric_uu(const Point &p) const = 0;

	// The following functions return the Christoffel and other derivative quantities of the metric.
	// They are implemented for this base class, BUT are left as virtual functions to allow for
	// other metrics to implement their own (more efficient)
	// way of calculating them, if so desired.
	//
	// Get the Christoffel symbol, indices up-down-down
	virtual ThreeIndex getChristoffel_udd(const Point &p) const;
	// Get the Riemann tensor, indices up-down-down-down
	virtual FourIndex getRiemann_uddd(const Point &p) const;
	// Get the Kretschmann scalar
	virtual real getKretschmann(const Point &p) const;

	// Function to get the description of the metric
	// (used for outputting to the screen while running and possibly to the output files)
	// There is a base class implementation of this function returning an undescriptive string
	virtual std::string getFullDescriptionStr() const;

	bool getrLogScale() const;

protected:
	//! The symmetries (coordinate Killing vectors) of the metric. Should be set by descendant constructor.
	std::vector<int> m_Symmetries{};
	//! Are we using a logarithmic r coordinate?
	const bool m_rLogScale;
};

/**
 * @brief Abstract base class for a metric with a spherical horizon (i.e. horizon at constant radius r)
 */
class SphericalHorizonMetric : public Metric
{
public:
	// No default construction allowed, must specify horizon radius
	SphericalHorizonMetric() = delete;
	// Constructor that initializes horizon radius
	SphericalHorizonMetric(real HorizonRadius, bool rLogScale);

	// Getter functions for the two member variables
	real getHorizonRadius() const;

protected:
	//! Radius of the horizon
	const real m_HorizonRadius;
};

/**
 * @brief The Kerr metric
 */
class KerrMetric final : public SphericalHorizonMetric
{
private:
	//! Mass-rescaled rotation parameter for Kerr. Note that this should be between -1 and 1.
	const real m_aParam;

	//! Mass parameter for Kerr. Default is 1.
	const real m_mParam;

public:
	// No default constructor allowed, must specify a
	KerrMetric() = delete;

	// Constructor setting parameter a
	KerrMetric(real aParam, bool rLogScale = false, real mParam = 1.);

	// The override of the basic metric getter functions
	TwoIndex getMetric_dd(const Point &p) const final;
	TwoIndex getMetric_uu(const Point &p) const final;

	// The override of the description string getter
	std::string getFullDescriptionStr() const final;
};

/**
 * @brief Flat space metric in 4D
 */
class FlatSpaceMetric final : public Metric
{
public:
	// Simple (default) constructor is all that is needed
	FlatSpaceMetric(bool rlogscale = false);

	// The override of the basic metric getter functions
	TwoIndex getMetric_dd(const Point &p) const final;
	TwoIndex getMetric_uu(const Point &p) const final;

	// The override of the description string getter
	std::string getFullDescriptionStr() const final;
};

/**
 * @brief Rasheed-Larsen black hole
 */
class RasheedLarsenMetric final : public SphericalHorizonMetric
{
private:
	//! a parameter for the Rasheed-Larsen metric
	const real m_aParam;
	//! m parameter for the Rasheed-Larsen metric
	const real m_mParam;
	//! p parameter for the Rasheed-Larsen metric
	const real m_pParam;
	//! q parameter for the Rasheed-Larsen metric
	const real m_qParam;

public:
	// No default constructor allowed, must specify parameters
	RasheedLarsenMetric() = delete;

	// Constructor setting parameter a
	RasheedLarsenMetric(real mParam, real aParam, real pParam, real qParam, bool rLogScale = false);

	// The override of the basic metric getter functions
	TwoIndex getMetric_dd(const Point &p) const final;
	TwoIndex getMetric_uu(const Point &p) const final;

	// The override of the description string getter
	std::string getFullDescriptionStr() const final;
};

/**
 * @brief Johannsen black hole metric
 * @note Implementation by Seppe Staelens
 */
class JohannsenMetric final : public SphericalHorizonMetric
{
private:
	// Johannsen up to first order in deviation function is specified by five parameters (if M=1)
	//! a parameter for the Johannsen metric
	const real m_aParam;
	//! alpha13 parameter for the Johannsen metric
	const real m_alpha13Param;
	//! alpha22 parameter for the Johannsen metric
	const real m_alpha22Param;
	//! alpha52 parameter for the Johannsen metric
	const real m_alpha52Param;
	//! eps3 parameter for the Johannsen metric
	const real m_eps3Param;

public:
	// No default constructor allowed, must specify parameters
	JohannsenMetric() = delete;

	// Constructor setting parameter a
	JohannsenMetric(real aParam, real alpha13Param, real alpha22Param, real alpha52Param, real eps3Param, bool rLogScale = false);

	// The override of the basic metric getter functions
	TwoIndex getMetric_dd(const Point &p) const final;
	TwoIndex getMetric_uu(const Point &p) const final;

	// The override of the description string getter
	std::string getFullDescriptionStr() const final;
};

/**
 * @brief Mano-Novikov metric (with angular momentum and M3 parameter turned on)
 * @note Implementation by Seppe Staelens
 */
class MankoNovikovMetric final : public SphericalHorizonMetric
{
private:
	// Manko-Novikov metric with only alpha3 as symmetry breaking parameter
	//! a parameter for the Manko-Novikov metric
	const real m_aParam;
	//! alpha3 parameter for the Manko-Novikov metric
	const real m_alpha3Param;

	// These are convenient derived quantities from a
	//! Derived alpha parameter
	const real m_alphaParam;
	//! Derived k parameter
	const real m_kParam;

public:
	// No default constructor allowed, must specify parameters
	MankoNovikovMetric() = delete;

	// Constructor setting parameter a and alpha3
	MankoNovikovMetric(real aParam, real alpha3Param, bool rLogScale = false);

	// The override of the basic metric getter functions
	TwoIndex getMetric_dd(const Point &p) const final;
	TwoIndex getMetric_uu(const Point &p) const final;

	// The override of the description string getter
	std::string getFullDescriptionStr() const final;
};

/**
 * @brief Kerr metric in Kerr-Schild coordinates
 * @note Normalized so that M = 1
 */
class KerrSchildMetric final : public SphericalHorizonMetric
{
private:
	//! Rotation parameter for Kerr. Note that this should be between -1 and 1 since M=1
	const real m_aParam;

public:
	// No default constructor allowed, must specify a
	KerrSchildMetric() = delete;

	// Constructor setting parameter a
	KerrSchildMetric(real aParam, bool rLogScale = false);

	// The override of the basic metric getter functions
	TwoIndex getMetric_dd(const Point &p) const final;
	TwoIndex getMetric_uu(const Point &p) const final;

	// The override of the description string getter
	std::string getFullDescriptionStr() const final;
};

/**
 * @brief Abstract base class for a metric with an arbitrary number of singularities (of arbitrary codimension)
 */
class SingularityMetric : public Metric
{
public:
	// Constructor that initializes singularities
	SingularityMetric(std::vector<Singularity> thesings, bool rLogScale);

	// Getter functions for the two member variables
	std::vector<Singularity> getSingularities() const;

protected:
	//! All singularities of the metric
	const std::vector<Singularity> m_AllSingularities;
};

/**
 * @brief Ring fuzzball metric
 * @note Implementation by Lies Van Dael
 */
class ST3CrMetric final : public SingularityMetric
{
public:
	// Constructor which will be called to initialize all parameters of the metric
	ST3CrMetric(real P, real q0, real lambda, bool rlogscale = false);

	// The basic getter functions
	TwoIndex getMetric_dd(const Point &p) const final;
	TwoIndex getMetric_uu(const Point &p) const final;

	// The description string getter
	std::string getFullDescriptionStr() const final;

private:
	const real m_P;
	const real m_q0;
	const real m_lambda;

	real get_omega(real r, real theta, real l) const;
	real f_phi(real phi, real r, real theta, real l, real R) const;
	real f_om_phi(real phi, real r, real theta, real l, real R) const;
};

/**
 * @brief Boson star metric with solitonic potential
 * @note Implementation by Seppe Staelens
 * @note The default files correspond to sigma = 0.06 and phi_c = 0.044
 */
class BosonStarMetric final : public Metric
{
public:
	// Simple (default) constructor is all that is needed
	BosonStarMetric(double Phi_infinity, int num_lines, bool rLogScale = false,
					std::string Phi_filename = "Phi.dat", std::string m_filename = "m.dat");
	// The override of the basic metric getter functions
	TwoIndex getMetric_dd(const Point &p) const final;
	TwoIndex getMetric_uu(const Point &p) const final;
	// The override of the description string getter
	std::string getFullDescriptionStr() const final;

protected:
	//! The value of Phi at infinity, before rescaling.
	const double m_Phi_infinity;
	//! Number of lines to be read from the data files. Mainly to avoid reading
	//! the data at extreme distances, which could upset the spline interpolation.
	const int m_num_lines;

	//! filename with Phi data
	std::string m_Phi_filename;
	//! filename with m data
	std::string m_m_filename;

	//! The spline interpolator for Phi
	tk::spline m_PhiSpline;
	//! The spline interpolator for m
	tk::spline m_mSpline;

	//! function to read the data
	void read_data();
};

class RotatingBosonStarMetric final : public Metric
{
public:
	// Simple (default) constructor is all that is needed
	RotatingBosonStarMetric(bool rLogScale = false, std::string MetricFolder = "RotatingBosonStar/data_Will/", int num_x = 500, int num_th = 399, real L = 1.);

	// The override of the basic metric getter functions
	TwoIndex getMetric_dd(const Point &p) const final;
	TwoIndex getMetric_uu(const Point &p) const final;
	// The override of the description string getter
	std::string getFullDescriptionStr() const final;

	// protected:
	//! The grids with the metric functions
	// Grid *m_grid_f;
	// Grid *m_grid_l;
	// Grid *m_grid_g;
	// Grid *m_grid_Omega;

	//! The grid interpolators
	BicubicSplineInterpolator *m_fInterpolator;
	BicubicSplineInterpolator *m_lInterpolator;
	BicubicSplineInterpolator *m_gInterpolator;
	BicubicSplineInterpolator *m_OmegaInterpolator;

	const real m_L;

	//! The interpolator for the metric functions
	// Interpolator *m_interpolator;
};

//// METRIC ADD POINT A ////
// Declare your new Metric class here, publically inheriting from the base class Metric
// (or SphericalHorizonMetric if your Metric has a horizon, or SingularityMetric if your Metric has other, arbitrary singularities)
// Give definitions (implementation) of these functions in Metric.cpp (or other source code file)
// Don't forget to set m_Symmetries appropriately (in the constructor),
// if your metric has any symmetry (e.g. stationarity, axisymmetry)!
// Sample code:
/*
class MyMetric final : public Metric // good practice to make the class final unless descendant classes are possible
{
public:
	// Constructor which will be called to initialize all parameters of the metric
	MyMetric(args...);

	// The basic getter functions
	// These MUST be implemented
	TwoIndex getMetric_dd(const Point& p) const final;
	TwoIndex getMetric_uu(const Point& p) const final;

	// The description string getter
	// This is optional (but recommended) to implement; if not implemented,
	// the base class Metric::getFullDescriptionStr() will be called instead
	std::string getFullDescriptionStr() const final;

private:
	// good practice to have all const params (initialized in the constructor)
	// since the metric cannot change after initialization
	// const params...;

};
*/
//// END METRIC ADD POINT A ////

#endif
