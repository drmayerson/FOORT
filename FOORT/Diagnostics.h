#ifndef _FOORT_DIAGNOSTICS_H
#define _FOORT_DIAGNOSTICS_H

///////////////////////////////////////////////////////////////////////////////////////
////// DIAGNOSTICS.H
////// Declarations of abstract base Diagnostic class and all its descendants.
////// All definitions in Diagnostics.cpp
///////////////////////////////////////////////////////////////////////////////////////

#include "Geometry.h" // for tensors

#include <cstdint> // for std::uint16_t
#include <string> // for strings
#include <memory> // for std::unique_ptr
#include <vector> // for std::vector

// Forward declaration of Geodesic class needed here, since Diagnostics are passed a pointer to their owner Geodesic
// (note "Geodesic.h" is NOT included to avoid header loop, and we do not need Geodesic member functions here!)
class Geodesic;

////////////////////////
//// DIAGNOSTIC BITFLAGS

// Diagnostic bitflags
// Used for constructing vector of Diagnostics
// Note that this means every diagnostic is either "on" or "off";
// it is not possible to have a Diagnostic "on" more than once
// Note: why not std::bitset<size>? Because in this way we can use expressions with DiagBitflag in conditional expressions,
// e.g. if ( mydiagbitflag & Diag_FourColorScreen)
using DiagBitflag = std::uint16_t;

// Define a bitflag per existing diagnostic
constexpr DiagBitflag Diag_None					{ 0b0000'0000'0000'0000 };
constexpr DiagBitflag Diag_GeodesicPosition		{ 0b0000'0000'0000'0001 };
constexpr DiagBitflag Diag_FourColorScreen		{ 0b0000'0000'0000'0010 };
constexpr DiagBitflag Diag_EquatorialPasses 	{ 0b0000'0000'0000'0100 };

//// DIAGNOSTIC ADD POINT B ////
// Add a DiagBitflag for your new diagnostic. Make sure you use a bitflag that has not been used before!
// Sample code:
/*
constexpr DiagBitflag Diag_MyDiag				{ 0b0000'0000'0000'1000 };
*/
//// END DIAGNOSTIC ADD POINT B ////


// This carries the information for a Diagnostic to update itself: if UpdateNsteps > 0, then every so many steps.
// If UpdateNSteps == 0, then it only updates at the start and/or finish of integration if the appropriate bool is set.
struct UpdateFrequency
{
	largecounter UpdateNSteps{ 0 };
	bool UpdateStart{ false };
	bool UpdateFinish{ false };
};


//////////////////////////////////////////////////////////////
//// GENERAL DECLARATIONS OF AND WITH ABSTRACT BASE CLASS ////

// Abstract base class for all diagnostics
class Diagnostic
{
public:
	// Constructor must initialize the pointer to its owner Geodesic
	Diagnostic() = delete;
	Diagnostic(Geodesic* const theGeodesic) : m_OwnerGeodesic{ theGeodesic }
	{}

	// virtual destructor to ensure correct destruction of descendants
	virtual ~Diagnostic() = default;

	// This is the heart of the Diagnostic. It calls the helper function DecideUpdate(), and if this
	// returns true, will update its internal status based on the current (new) state of its owner geodesic.
	virtual void UpdateData() = 0;

	// These functions are for use at the end of integration of a geodesic.
	// getFullData() returns all the data stored in the Diagnostic as a string (for output to file)
	virtual std::string getFullDataStr() const = 0;
	// getFinalDataVal() associates a value to the geodesic corresponding to the final value of this diagnostic
	// (used for determining coarseness of nearby geodesics)
	virtual std::vector<real> getFinalDataVal() const = 0;

	// Function used to determine distance between two values obtained from getFinalDataVal()
	// (for determining coarseness of nearby geodesics)
	// This should return a number >=0
	virtual real FinalDataValDistance(const std::vector<real>& val1, const std::vector<real>& val2) const = 0;

	// Getters for descriptions
	// This returns the name (only) of the Diagnostic, as a string without spaces that will be appended
	// to an output file (e.g. prefix_DiagName.ext). Must be implemented!
	virtual std::string getNameStr() const = 0;
	// This returns the full description of the Diagnostic. Default implementation
	// returns GetDiagNameStr()
	virtual std::string getFullDescriptionStr() const;

protected:
	// The geodesic that owns the Diagnostic (a const pointer to the Geodesic)
	Geodesic* const m_OwnerGeodesic;

	// Helper function to decide if the Diagnostic should indeed update its status, based on
	// its UpdateFrequency struct information (which will come from the Diagnostics's DiagnosticOptions)
	bool DecideUpdate(const UpdateFrequency& myUpdateFrequency);

	// The diagnostic is itself in charge of keeping track of how many steps it has been since it has been updated
	// The Diagnostic's DiagnosticOptions struct tells it how many steps it needs to wait between updates
	largecounter m_StepsSinceUpdated{};
};

// Owner vector of derived Diagnostics classes
using DiagnosticUniqueVector = std::vector<std::unique_ptr<Diagnostic>>;

// Helper to create a new vector of Diagnostic options, based on the bitflag
// The first diagnostic is the value diagnostic
DiagnosticUniqueVector CreateDiagnosticVector(DiagBitflag diagflags, DiagBitflag valdiag, Geodesic* const theGeodesic);



//////////////////////////////////////////////////////
//// DECLARATIONS FOR DERIVED DIAGNOSTIC CLASSES  ////


// The four color screen: associates one of four colors based on the quadrant that the geodesic finishes in
// Will ONLY return a color if the geodesic indeed finishes because it passes through the boundary sphere!
class FourColorScreenDiagnostic final : public Diagnostic
{
public:
	// Basic constructor only passes on Geodesic pointer to base class constructor
	FourColorScreenDiagnostic(Geodesic* const theGeodesic) : Diagnostic(theGeodesic) {}

	// Sets the quadrant to the appropriate value, IF the boundary sphere has been reached
	void UpdateData() override;

	// Both of these output functions simply returns the quadrant number associated with the geodesic's end position
	std::string getFullDataStr() const final;
	std::vector<real> getFinalDataVal() const final;

	// Discrete metric for distance: returns 0 if the quadrants are the same, 1 if they are not
	real FinalDataValDistance(const std::vector<real>& val1, const std::vector<real>& val2) const final;

	// Description string getters
	std::string getNameStr() const final;
	std::string getFullDescriptionStr() const final;

	// FourColorScreen does not need any (static) options!

private:
	// Note initialization to 0; this means the default value returned will be 0
	// (e.g. if Term::BoundarySphere is not reached)
	int m_quadrant{0};
};


// Forward declaration needed before Diagnostic
struct GeodesicPositionOptions;
// Geodesic position tracker
class GeodesicPositionDiagnostic final : public Diagnostic
{
public:
	// Basic constructor only passes on Geodesic pointer to base class constructor
	GeodesicPositionDiagnostic(Geodesic* const theGeodesic) : Diagnostic(theGeodesic) {}

	// Stores the current position of the geodesic
	void UpdateData() final;

	// This returns as many stored positions as is specified in the options struct
	std::string getFullDataStr() const final;
	// This returns the final (theta,phi) value of the geodesic
	std::vector<real> getFinalDataVal() const final;

	// This determines the angular distance between two geodesic
	// (based on their final angles on the boundary sphere (theta, phi))
	real FinalDataValDistance(const std::vector<real>& val1, const std::vector<real>& val2) const final;

	// Description string getters
	std::string getNameStr() const final;
	std::string getFullDescriptionStr() const final;

	// The options: specifies the update frequency but also how many points we want to output in the end
	static std::unique_ptr<GeodesicPositionOptions> DiagOptions;

private:
	// Keeps track of the points that are saved
	std::vector<Point> m_AllSavedPoints{};
};


// Forward declaration needed before Diagnostic
struct DiagnosticOptions;
// Diagnostic for counting number of passes through equatorial plane
class EquatorialPassesDiagnostic final : public Diagnostic
{
public:
	// Basic constructor only passes on Geodesic pointer to base class constructor
	EquatorialPassesDiagnostic(Geodesic* const theGeodesic) : Diagnostic(theGeodesic) {}

	// Checks to see if we have a new cross over the equatorial plane
	void UpdateData() final;

	// Returns the number of passes over the equatorial plane
	std::string getFullDataStr() const final;
	std::vector<real> getFinalDataVal() const final;

	// Simple absolute value of difference of passes
	real FinalDataValDistance(const std::vector<real>& val1, const std::vector<real>& val2) const final;

	// Description string getters
	std::string getNameStr() const final;
	std::string getFullDescriptionStr() const final;

	// Only needs the basic UpdateFrequency options
	static std::unique_ptr<DiagnosticOptions> DiagOptions;

private:
	// Keeps track of how many passes have been made
	int m_EquatPasses{ 0 };

	// Keeps track of the previous theta angle, so that we can compare with current theta angle
	real m_PrevTheta{ -1 };
};

//// DIAGNOSTIC ADD POINT A1 ////
// Declare your Diagnostic class here, inheriting from Diagnostic.
// Sample code:
/*
// Forward declaration needed before Diagnostic (if using base class DiagnosticOptions, this is strictly speaking not necessary)
struct DiagnosticOptions;
// class definition
class MyDiagnostic final : public Diagnostic // good practice to make the class final unless descendant classes are possible
{
	// constructor must at least take and pass along the const pointer to the owner Geodesic
	MyDiagnostic(Geodesic* const theGeodesic) : Diagnostic(theGeodesic) {}

	// This is the heart of the Diagnostic: here the Diagnostic updates its internal state according to
	// the owner Geodesic's current state
	void UpdateData() final;

	// This should return the string that is to be outputted to the file as the final output of this Diagnostic
	// for its owner Geodesic
	std::string getFullDataStr() const final;
	// This should return a vector of real numbers that indicates the final "value" that should be associated to
	// the owner Geodesic --- this is then used in FinalDataValDistance() to find "distances" between geodesics.
	std::vector<real> getFinalDataVal() const final;

	// This should return a (positive) distance of two values returned by getFinalDataVal(), indicated the
	// "distance" of two geodesics (this is used for Mesh refinement)
	real FinalDataValDistance(const std::vector<real>& val1, const std::vector<real>& val2) const final;

	// Must implement getNameStr() (and recommended also to implement getFullDescriptionStr)
	// getNameStr() is a simple, short string (without spaces) that will be appended to the file name
	// where this Diagnostic's output is written. getFullDescriptionStr() is a descriptive string that should list
	// all relevant options set; this is outputted to e.g. the screen at runtime.
	std::string getNameStr() const final;
	std::string getFullDescriptionStr() const final;

	// Diagnostic options: basic DiagnosticOptions only contains UpdateFrequency information,
	// if needed, can use a descendant class with more options (see e.g. GeodesicPositionDiagnostic)
	static std::unique_ptr<DiagnosticOptions> DiagOptions;

private:
	// will probably want some private member variable(s) to keep track of whatever geodesic property is desired
};
*/
//// END DIAGNOSTIC ADD POINT A1 ////



///////////////////////////////////////
//// ALL DIAGNOSTICOPTIONS STRUCTS ////

// Base class for DiagnosticOptions. Other Diagnostics can inherit from here if they require more options.
struct DiagnosticOptions
{
public:
	// Basic constructor only sets the number of steps between updates
	DiagnosticOptions(UpdateFrequency thefrequency) : theUpdateFrequency{ thefrequency }
	{}

	virtual ~DiagnosticOptions() = default;

	const UpdateFrequency theUpdateFrequency;
};

// GeodesicPositionDiagnostic needs more options
struct GeodesicPositionOptions : public DiagnosticOptions
{
public:
	GeodesicPositionOptions(largecounter outputsteps, UpdateFrequency thefrequency) : OutputNrSteps{ outputsteps }, 
		DiagnosticOptions(thefrequency)
	{}

	const largecounter OutputNrSteps;
};

//// DIAGNOSTIC ADD POINT A2 (optional) ////
// if necessary, define your new DiagnosticOptions class here
// Sample code:
/*
struct MyDiagnosticOptions : public DiagnosticOptions
{
public:
	// Constructor should pass along UpdateFrequency information to base class
	MyDiagnosticOptions(UpdateFrequency thefrequency) : DiagnosticOptions(thefrequency) //, (...) other initializations
	{}

	// other member variables here - make them const!
	// ...
};
*/
//// END DIAGNOSTIC ADD POINT A2 ////


#endif
