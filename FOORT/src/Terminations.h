#ifndef _FOORT_TERMINATIONS_H
#define _FOORT_TERMINATIONS_H

/**
 * @file Terminations.h
 * @author Daniel R. Mayerson
 * @brief Declarations of abstract base Termination class and all its descendants.
 * @version 0.1
 * @date 2025-01-14
 * @copyright Copyright (c) 2025
 */

#include "Geometry.h" // for basic tensor objects

#include <cstdint> // for std::uint16_t
#include <memory>  // for std::unique_ptr
#include <vector>  // for std::vector
#include <utility> // for std::pair

// Forward declaration of Geodesic class needed here, since Diagnostics are passed a pointer to their owner Geodesic
// (note "Geodesic.h" is NOT included to avoid header loop, and we do not need Geodesic member functions here!)
class Geodesic;

// TERMINATION BITFLAGS
//! Used for constructing vector of Terminations. Note that this means every Termination is either "on" or "off"; it is not possible to have a Termination "on" more than once
using TermBitflag = std::uint16_t;

// Define a bitflag per existing Termination
//! No terminations bitflag
constexpr TermBitflag Term_None{0b0000'0000'0000'0000};
//! Boundary Sphere termination bitflag
constexpr TermBitflag Term_BoundarySphere{0b0000'0000'0000'0001};
//! Time Out termination bitflag
constexpr TermBitflag Term_TimeOut{0b0000'0000'0000'0010};
//! Horizon termination bitflag
constexpr TermBitflag Term_Horizon{0b0000'0000'0000'0100};
//! Theta Singularity termination bitflag
constexpr TermBitflag Term_ThetaSingularity{0b0000'0000'0000'1000};
//! NaN termination bitflag
constexpr TermBitflag Term_NaN{0b0000'0000'0001'0000};
//! General Singularity termination bitflag
constexpr TermBitflag Term_GeneralSingularity{0b0000'0000'0010'0000};

//// TERMINATION ADD POINT B1 ////
// Add a TermBitflag for your new Termination. Make sure you use a bitflag that has not been used before!
// Sample code:
/*
constexpr TermBitflag Term_MyTerm				{ 0b0000'0000'0000'1000 };
*/
//// END TERMINATION ADD POINT B1 ////

// TERMINATION CONDITIONS

/**
 * @brief Possible termination conditions that can be set by Terminations
 */
enum class Term
{
	//! Geoedesic has not been properly initialized yet with initial position/velocity
	Uninitialized = -1,
	//! All is right, continue integrating geodesic
	Continue = 0,
	//! STOP, encountered horizon (set by HorizonTermination)
	Horizon,
	//! STOP, encountered boundary sphere (set by BoundarySphereTermination)
	BoundarySphere,
	//! STOP, taken too many steps (set by TimeOutTermination)
	TimeOut,
	//! STOP, too close to polar coordinate singularity (theta = 0 or theta = pi/2) (set by ThetaSingularityTermination)
	ThetaSingularity,
	//! STOP, NaN encountered in geodesic position or velocity (set by NaNTermination)
	NaN,
	//! STOP, singularity encountered (of any codimension) (set by GeneralSingularityTermination)
	GeneralSingularity,

	//// TERMINATION ADD POINT B2 ////
	// Add a new Termination condition that your new Termination can set
	// Sample code:
	/*
	MyTermCond,			// STOP, encountered (...)
	*/
	//// END TERMINATION ADD POINT B2 ////

	Maxterms // Number of termination conditions that exist
};

// GENERAL DECLARATIONS OF ABSTRACT BASE CLASS

/**
 * @brief Abstract base class for all Terminations.
 */
class Termination
{
public:
	// Constructor must initialize the pointer to its owner Geodesic
	Termination() = delete;
	Termination(Geodesic *const theGeodesic) : m_OwnerGeodesic{theGeodesic}
	{
	}

	// Resets Termination object. This is called when the owner Geodesic is reset in order to start integrating
	// a new geodesic.
	// The base class implementation only resets m_StepsSinceUpdated
	// Descendants can override this if they need to reset additional internal variables
	virtual void Reset();

	// virtual destructor to ensure correct destruction of descendants
	virtual ~Termination() = default;

	// Function that is called to determine whether Termination wants to
	// terminate the Geodesic. Returns Term::Continue if no termination wanted,
	// otherwise it returns the appropriate Term condition
	virtual Term CheckTermination() = 0;

	// This returns the full description of the Termination
	virtual std::string getFullDescriptionStr() const = 0;

protected:
	//! The geodesic that owns the Termination (a const pointer to the Geodesic)
	Geodesic *const m_OwnerGeodesic;

	// Helper function to decide if the Termination should indeed update its status, based on
	// UpdateNSteps (which is set to 0 if we always update)
	bool DecideUpdate(largecounter UpdateNSteps);

	//! The termination is itself in charge of keeping track of how many steps it has been since it has been updated. The Termination's TerminationOptions struct tells it how many steps it needs to wait between updates
	largecounter m_StepsSinceUpdated{};
};

//! The owner vector of derived Termination classes
using TerminationUniqueVector = std::vector<std::unique_ptr<Termination>>;

// Helper to create a new vector of Termination options, based on the bitflag
TerminationUniqueVector CreateTerminationVector(TermBitflag termflags, Geodesic *const theGeodesic);

// DECLARATIONS FOR DERIVED TERMINATION CLASSES (DESCENDANTS)

// Forward declaration needed before Termination
struct HorizonTermOptions;

/**
 * @brief HorizonTermination: Terminate geodesics if they get too close to the horizon
 */
class HorizonTermination final : public Termination
{
public:
	// Basic constructor only passes on Geodesic pointer to base class constructor
	HorizonTermination(Geodesic *const theGeodesic) : Termination(theGeodesic) {}

	// Check if we are too close to the horizon
	Term CheckTermination() final;

	// Description string
	std::string getFullDescriptionStr() const final;

	//! Options (contains horizon radius, if we are using logarithmic r coordinate, and distance allowed from the horizon)
	static std::unique_ptr<HorizonTermOptions> TermOptions;
};

// Forward declaration needed before Termination
struct BoundarySphereTermOptions;

/**
 * @brief BoundarySphereTermination: Terminate geodesics if they reach outside of a boundary sphere
 */
class BoundarySphereTermination final : public Termination
{
public:
	// Basic constructor only passes on Geodesic pointer to base class constructor
	BoundarySphereTermination(Geodesic *const theGeodesic) : Termination(theGeodesic) {}

	// Check if we have passed the boundary sphere
	Term CheckTermination() final;

	// Description string
	std::string getFullDescriptionStr() const final;

	//! The options that the BoundarySphereTermination keeps (contains the radius of the boundary sphere)
	static std::unique_ptr<BoundarySphereTermOptions> TermOptions;
};

// Forward declaration needed before Termination
struct TimeOutTermOptions;

/**
 * @brief TimeOutTermination: Terminate geodesics if they take too many steps
 */
class TimeOutTermination final : public Termination
{
public:
	// Basic constructor only passes on Geodesic pointer to base class constructor
	TimeOutTermination(Geodesic *const theGeodesic) : Termination(theGeodesic) {}

	// This descendant needs to override Reset in order to also reset m_CurNrSteps
	void Reset() final;

	// Check if we have already taken too many steps
	Term CheckTermination() final;

	// Description string
	std::string getFullDescriptionStr() const final;

	//! The options that the TimeOutTermination keeps (contains max number of steps allowed)
	static std::unique_ptr<TimeOutTermOptions> TermOptions;

private:
	//! Keep track of the number of steps that the geodesic has taken so far
	largecounter m_CurNrSteps{0};
};

// Forward declaration needed before Termination
struct ThetaSingularityTermOptions;

/**
 * @brief ThetaSingularityTermination: Terminate geodesics if they get too close to a polar coordinate singularity
 */
class ThetaSingularityTermination final : public Termination
{
public:
	ThetaSingularityTermination(Geodesic *const theGeodesic) : Termination(theGeodesic) {}

	// No override of Reset() necessary

	// Check the specific termination condition
	Term CheckTermination() final;

	// Description string
	std::string getFullDescriptionStr() const final;

	//! The options that the Termination keeps (will probably be a descendant struct instead, which specifies any additional options the Termination needs)
	static std::unique_ptr<ThetaSingularityTermOptions> TermOptions;
};

// Forward declaration needed before Termination
struct NaNTermOptions;

/**
 * @brief NaNTermination: Terminate geodesics if they contain a NaN in position or velocity
 */
class NaNTermination final : public Termination
{
public:
	// Basic constructor only passes on Geodesic pointer to base class constructor
	NaNTermination(Geodesic *const theGeodesic) : Termination(theGeodesic) {}

	// Check if we are too close to the horizon
	Term CheckTermination() final;

	// Description string
	std::string getFullDescriptionStr() const final;

	//! Options (contains horizon radius, if we are using logarithmic r coordinate, and distance allowed from the horizon)
	static std::unique_ptr<NaNTermOptions> TermOptions;
};

// Forward declaration needed before Termination
struct GeneralSingularityTermOptions;

/**
 * @brief GeneralSingularityTermination: Terminate geodesics if they get too close to one of a number of singularities (of arbitrary codimension)
 */
class GeneralSingularityTermination final : public Termination
{
public:
	// Basic constructor only passes on Geodesic pointer to base class constructor
	GeneralSingularityTermination(Geodesic *const theGeodesic) : Termination(theGeodesic) {}

	// Check if we are too close to the horizon
	Term CheckTermination() final;

	// Description string
	std::string getFullDescriptionStr() const final;

	//! Options (contains horizon radius, if we are using logarithmic r coordinate, and distance allowed from the horizon)
	static std::unique_ptr<GeneralSingularityTermOptions> TermOptions;

private:
	std::string SingularityToString(int singnr) const;
};

//// TERMINATION ADD POINT A1 /////
// Declare your Termination class here, inheriting from Termination.
// Sample code:
/*
// Forward declaration needed before Termination
struct TerminationOptions; // possibly will instead need to declare descendant options struct
class MyTermination final : public Termination // good practice to make the class final unless descendant classes are possible
{
public:
	// Constructor must at least pass on Geodesic pointer to base class constructor
	MyTermination(Geodesic* const theGeodesic) : Termination(theGeodesic) {}

	// Do you need to reset any internal variables specific to MyTermination? If so, override Reset() (This is not mandatory)
	// Note: make sure to call the base class implementation Termination::Reset()
	// from within your implementation of MyTermination::Reset(), so that the base class internal variable is also reset!
	void Reset() final;

	// Check the specific termination condition
	Term CheckTermination() final;

	// Description string
	std::string getFullDescriptionStr() const final;

	// The options that the Termination keeps (will probably be a descendant struct instead, which specifies
	// any additional options the Termination needs)
	static std::unique_ptr<TerminationOptions> TermOptions;

private:
	// any private member variables that are needed to keep track of things
};
*/
//// END TERMINATION ADD POINT A1 ////

// ALL TERMINATIONOPTIONS STRUCTS

/**
 * @brief Base class for all TerminationOptions structs. Other TerminationOptions can inherit from here if they require more options.
 */
struct TerminationOptions
{
public:
	// Basic constructor only sets the number of steps between updates
	TerminationOptions(largecounter Nsteps) : UpdateEveryNSteps{Nsteps}
	{
	}

	// virtual destructor to ensure correct destruction of descendants
	virtual ~TerminationOptions() = default;

	//! Number of steps between updates
	const largecounter UpdateEveryNSteps;
};

/**
 * @brief Options for HorizonTermination.
 * @details Keeps track of the horizon radius, whether we are using a logarithmic r coordinate, and the distance allowed from the horizon.
 */
struct HorizonTermOptions : public TerminationOptions
{
public:
	HorizonTermOptions(real theHorizonRadius, bool therLogScale, real theAtHorizonEps, largecounter Nsteps) : HorizonRadius{theHorizonRadius}, AtHorizonEps{theAtHorizonEps}, rLogScale{therLogScale}, TerminationOptions(Nsteps)
	{
	}

	//! The radius of the horizon
	const real HorizonRadius;
	//! The distance allowed from the horizon
	const real AtHorizonEps;
	//! Whether we are using a logarithmic r coordinate
	const bool rLogScale;
};

/**
 * @brief Options for BoundarySphereTermination.
 * @details Keeps track of the boundary sphere radius, whether we are using a logarithmic r coordinate.
 */
struct BoundarySphereTermOptions : public TerminationOptions
{
public:
	BoundarySphereTermOptions(real theRadius, bool therLogScale, largecounter Nsteps) : SphereRadius{theRadius}, rLogScale{therLogScale},
																						TerminationOptions(Nsteps)
	{
	}

	//! Radius of the boundary sphere
	const real SphereRadius;
	//! Whether we are using a logarithmic r coordinate
	const bool rLogScale;
};

/**
 * @brief Options for TimeOutTermination
 * @details Keeps track of the max. number of integration steps allowed
 */
struct TimeOutTermOptions : public TerminationOptions
{
public:
	TimeOutTermOptions(largecounter MaxStepsAllowed, largecounter Nsteps) : MaxSteps{MaxStepsAllowed}, TerminationOptions(Nsteps)
	{
	}

	//! Max. number of integration steps allowed.
	const largecounter MaxSteps;
};

// Options class for ThetaSingularityTermination
/**
 * @brief Options for ThetaSingularityTermination
 * @details Keeps track of the minimum required distance from the singularity
 */
struct ThetaSingularityTermOptions : public TerminationOptions
{
public:
	ThetaSingularityTermOptions(real epsilon, largecounter Nsteps) : ThetaSingEpsilon{epsilon}, TerminationOptions(Nsteps)
	{
	}

	//! Minimum difference between theta and 0 or pi.
	const real ThetaSingEpsilon;
};

/**
 * @brief Options for NaNTermination
 */
struct NaNTermOptions : public TerminationOptions
{
public:
	NaNTermOptions(bool consoleoutputon, largecounter Nsteps) : OutputToConsole{consoleoutputon}, TerminationOptions(Nsteps)
	{
	}

	//! Whether to output to the console when a NaN is encountered
	const bool OutputToConsole;
};

/**
 * @brief Options for GeneralSingularityTermination
 * @details Keeps track of the singularities, allowed distance and whether we are using a logarithmic r coordinate.
 */
struct GeneralSingularityTermOptions : public TerminationOptions
{
public:
	GeneralSingularityTermOptions(std::vector<Singularity> sings,
								  real eps, bool consoleoutputon, bool therlogscale, largecounter Nsteps)
		: Singularities{std::move(sings)}, Epsilon{eps}, OutputToConsole{consoleoutputon},
		  rLogScale{therlogscale},
		  TerminationOptions(Nsteps) {}

	//! Vector of singularities
	const std::vector<Singularity> Singularities;
	//! Minimum flat distance the geodesic is allowed to be from any singularity
	const real Epsilon;
	//! Whether to output a message to the console when a singularity is encountered
	const bool OutputToConsole;
	//! Whether we are using a logarithmic r coordinate or not
	const bool rLogScale;
};

//// TERMINATION ADD POINT A2 ////
// Add your new TerminationOptions struct here, inheriting from TerminationOptions (if needed)
// Sample code:
/*
struct MyTermOptions : public TerminationOptions
{
public:
	MyTermOptions(..., largecounter Nsteps) : TerminationOptions(Nsteps) //, other initialization
	{}

	// member variables (const!) here
};
*/
//// END TERMINATION ADD POINT A2 ////

#endif
