#ifndef _FOORT_GEODESIC_H
#define _FOORT_GEODESIC_H

///////////////////////////////////////////////////////////////////////////////////////
////// GEODESIC.H
////// Declarations of abstract base Source class and all its descendants.
////// Declaration of Geodesic class.
////// All definitions in Geodesic.cpp
///////////////////////////////////////////////////////////////////////////////////////

#include "Geometry.h" // for tensor objects
#include "Metric.h" // for the metric
#include "Diagnostics.h" // Geodesics own Diagnostics
#include "Terminations.h" // Geodesics own Terminations
#include "Integrators.h" // Geodesics use an GeodesicIntegratorFunc to integrate itself

///////////////////////////////////////////////////////////
//// DECLARATIONS OF SOURCE BASE CLASS AND DESCENDANTS ////

// Abstract base class
class Source
{
public:
	// Constructor initializes Metric
	Source(const Metric* const theMetric) : m_theMetric{ theMetric } {}

	// Virtual destructor to ensure correct descendant destruction
	virtual ~Source() = default;

	// Get the source for the current geodesic position and velocity
	virtual OneIndex getSource(Point pos, OneIndex vel) const = 0;

	// Full description string (space allowed), to be outputted to file
	virtual std::string getFullDescriptionStr() const;

protected:
	// A const pointer to a const metric
	const Metric* const m_theMetric;
};

// NoSource: there is no source, i.e. the geodesic is indeed a geodesic (and feels no force)
class NoSource final : public Source
{
public:
	// Simple constructor passes on the Metric pointer to the base constructor
	NoSource(const Metric* const theMetric) : Source(theMetric) {}

	// Returns zero source
	OneIndex getSource(Point pos, OneIndex vel) const final;

	// Description string getter
	std::string getFullDescriptionStr() const final;
};

////////////////////////////////////////
//// DECLARATIONS OF GEODESIC CLASS ////

// Geodesic class: an instance of this class is created for each Geodesic that is integrated.
// The Geodesic is in charge of integrating itself until termination, updating its Diagnostics accordingly,
// and (after termination) returning the appropriate output.
class Geodesic
{
public:
	// Default constructor not allowed
	Geodesic() = delete;
	// Copy constructor or copy assignment not allowed
	Geodesic(const Geodesic&) = delete;
	Geodesic& operator=(const Geodesic&) = delete;

	// Constructor which initializes the Geodesic
	// Takes the following arguments which initialize the private member variables
	// - ScreenIndex
	// - Initial position
	// - Initial velocity
	// - Metric (pointer)
	// - Source (pointer)
	// - Diagnostic bitflag (& value Diagnostic bitflag) (used to create a vector of new instances of Diagnostics)
	// - Termination bitflag (used to create a vector of new instances of Terminations)
	// - Geodesic integrator function to use for integrating geodesic equation
	Geodesic(ScreenIndex scrindex, Point initpos, OneIndex initvel,
		const Metric* const theMetric, const Source* const theSource,
		DiagBitflag diagbit, DiagBitflag valdiagbit,
		TermBitflag termbit, GeodesicIntegratorFunc theIntegrator) : 
		m_ScreenIndex{ scrindex },
		m_CurrentPos { initpos }, m_CurrentVel{ initvel },
		m_theMetric{ theMetric }, m_theSource{ theSource },
		m_AllDiagnostics{ CreateDiagnosticVector(diagbit,valdiagbit,this) },
		m_AllTerminations{ CreateTerminationVector(termbit,this) },
		m_theIntegrator{ theIntegrator }
	{
		// Start: loop through all diagnostics to update at starting position
		for (const auto& d : m_AllDiagnostics)
		{
			d->UpdateData();
		}
	}

	// This makes the Geodesic integrate itself one step; then the Geodesic loops through all Terminations and Diagnostics to update
	Term Update();

	// Getters for properties of its internal state
	Term getTermCondition() const; // Current termination condition (Term::Continue if not done integrating)
	Point getCurrentPos() const; // Current position
	OneIndex getCurrentVel() const; // Current velocity
	real getCurrentLambda() const; // Current value of affine parameter

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
	Term m_TermCond{Term::Continue}; // As long as this is Term::Continue, not done integrating yet
	Point m_CurrentPos; // Current position
	OneIndex m_CurrentVel; // Current proper velocity 
	real m_curLambda{ 0.0 }; // Current value of affine parameter (starts at 0.0)


	// The Geodesic keeps track of what index it has been assigned;
	// it outputs this information in its final output string
	const ScreenIndex m_ScreenIndex;

	// These are const pointers (or const vectors of pointers) that contain all the information the Geodesic needs
	const Metric* const m_theMetric; // Metric is needed to evaluate the geodesic equation
	const Source* const m_theSource; // Source for the rhs of the geodesic equation
	// An instance of each Diagnostic and Termination is created for the Geodesic (in its constructor);
	// so the Geodesic is the owner of these objects.
	const DiagnosticUniqueVector m_AllDiagnostics;
	const TerminationUniqueVector m_AllTerminations;
	const GeodesicIntegratorFunc m_theIntegrator; // This is the function that will integrate the geodesic equation one step
};

#endif
