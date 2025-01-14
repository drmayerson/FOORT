#ifndef _FOORT_GEODESIC_H
#define _FOORT_GEODESIC_H

#include "Geometry.h"	  // for tensor objects
#include "Metric.h"		  // for the metric
#include "Diagnostics.h"  // Geodesics own Diagnostics
#include "Terminations.h" // Geodesics own Terminations
#include "Integrators.h"  // Geodesics use an GeodesicIntegratorFunc to integrate itself

#include <string> // for strings
#include <vector> // for std::vector

/**
 * @file Geodesic.h
 * @author Daniel R. Mayerson
 * @brief Declarations of abstract base Source class and all its descendants. Declaration of Geodesic class.
 * @version 1.0
 * @date 2024-12-16
 * @copyright Copyright (c) 2024
 */

///////////////////////////////////////////////////////////
//// DECLARATIONS OF SOURCE BASE CLASS AND DESCENDANTS ////

/**
 * @brief Abstract Source base class
 *
 */
class Source
{
public:
	// Constructor initializes Metric
	Source(const Metric *const theMetric) : m_theMetric{theMetric} {}

	// Virtual destructor to ensure correct descendant destruction
	virtual ~Source() = default;

	// Get the source for the current geodesic position and velocity
	virtual OneIndex getSource(Point pos, OneIndex vel) const = 0;

	// Full description string (space allowed), to be outputted to file
	virtual std::string getFullDescriptionStr() const;

protected:
	//! A const pointer to a const metric
	const Metric *const m_theMetric;
};

/**
 * @brief NoSource class.
 * @details There is no source, i.e. the geodesic is indeed a geodesic (and feels no force)
 *
 */
class NoSource final : public Source
{
public:
	// Simple constructor passes on the Metric pointer to the base constructor
	NoSource(const Metric *const theMetric) : Source(theMetric) {}

	// Returns zero source
	OneIndex getSource(Point pos, OneIndex vel) const final;

	// Description string getter
	std::string getFullDescriptionStr() const final;
};

////////////////////////////////////////
//// DECLARATIONS OF GEODESIC CLASS ////

/**
 * @brief Geodesic class: an instance of this class is created for each Geodesic that is integrated.
 * @details The Geodesic is in charge of integrating itself until termination, updating its Diagnostics accordingly,
 * and (after termination) returing the appropriate output
 *
 */
class Geodesic
{
public:
	// Default constructor not allowed
	Geodesic() = delete;
	// Copy constructor or copy assignment not allowed
	Geodesic(const Geodesic &) = delete;
	Geodesic &operator=(const Geodesic &) = delete;

	/**
	 * @brief Construct a new Geodesic object. Arguments initialize private member variables that remain the same over all geodesics.
	 *
	 * @param theMetric Pointer to the Metric object.
	 * @param theSource Pointer to the Source object.
	 * @param diagbit Diagnostic bitflag.
	 * @param valdiagbit Value diagnostic bitflag.
	 * @param termbit Termination bitflag.
	 * @param theIntegrator Geoedesic integrator function to use for integrating geodesic equation.
	 */
	Geodesic(const Metric *const theMetric, const Source *const theSource,
			 DiagBitflag diagbit, DiagBitflag valdiagbit,
			 TermBitflag termbit, GeodesicIntegratorFunc theIntegrator) : m_theMetric{theMetric}, m_theSource{theSource},
																		  m_AllDiagnostics{CreateDiagnosticVector(diagbit, valdiagbit, this)},
																		  m_AllTerminations{CreateTerminationVector(termbit, this)},
																		  m_theIntegrator{theIntegrator}
	{
	}

	// This initializes/resets the geodesic with a given ScreenIndex, initial position, and initial velocity
	// Also resets all Diagnostics and Terminations, resets the TermCondition to Term::Continue,
	// and puts the Geodesic back to lambda = 0.0.
	void Reset(ScreenIndex scrindex, Point initpos, OneIndex initvel);

	// This makes the Geodesic integrate itself one step; then the Geodesic loops through all Terminations and Diagnostics to update
	Term Update();

	// Getters for properties of its internal state
	Term getTermCondition() const;		// Current termination condition (Term::Continue if not done integrating)
	Point getCurrentPos() const;		// Current position
	OneIndex getCurrentVel() const;		// Current velocity
	real getCurrentLambda() const;		// Current value of affine parameter
	ScreenIndex getScreenIndex() const; // screen index

	// Output getters, to be called after the Geodesic terminates
	// This gets the complete output that should be written to the output files;
	// there is one string more than the count of Diagnostics: one string per Diagnostic,
	// PLUS the first string is the screen index.
	std::vector<std::string> getAllOutputStr() const;
	// This returns the "value" (from the Diagnostic that was set to the value Diagnostic) that is associated
	// to the Geodesic. Will be used to determine "distance" between Geodesics which is used in Mesh refinement.
	std::vector<real> getDiagnosticFinalValue() const;

private:
	// These variables define its internal state
	//! As long as this is Term::Continue, not done integrating yet
	Term m_TermCond{Term::Uninitialized};
	//! Current position
	Point m_CurrentPos{};
	//! Current proper velocity
	OneIndex m_CurrentVel{};
	//! Current value of affine parameter (starts at 0.0)
	real m_curLambda{0.0};

	//! The Geodesic keeps track of what index it has been assigned;
	//! it outputs this information in its final output string
	ScreenIndex m_ScreenIndex{};

	// These are const pointers (or const vectors of pointers) that contain all the information the Geodesic needs
	//! const pointer to the Metric
	const Metric *const m_theMetric;
	//! const pointer to the Source for the rhs of the geodesic equation
	const Source *const m_theSource;
	//! Vector of unique pointers to Diagnostics. An instance of each Diagnostic is created for the Geodesic, so the Geodesic is the owner of these objects.
	const DiagnosticUniqueVector m_AllDiagnostics;
	//! Vector of unique pointers to Terminations. An instance of each Termination is created for the Geodesic, so the Geodesic is the owner of these objects.
	const TerminationUniqueVector m_AllTerminations;
	//! This is the function that will integrate the geodesic equation one step
	const GeodesicIntegratorFunc m_theIntegrator;
};

#endif
