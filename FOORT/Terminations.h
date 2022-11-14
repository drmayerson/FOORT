#ifndef _FOORT_TERMINATIONS_H
#define _FOORT_TERMINATIONS_H

#include<vector>
#include<memory>

#include"Metric.h"

// Termination bitflags
// Used for constructing vector of Terminations
// Note that this means every Termination is either "on" or "off";
// it is not possible to have a Termination "on" more than once
using TermBitflag = std::uint16_t;

constexpr TermBitflag Term_None{ 0b0000'0000'0000'0000 };
constexpr TermBitflag Term_BoundarySphere{ 0b0000'0000'0000'0001 };
constexpr TermBitflag Term_TimeOut{ 0b0000'0000'0000'0010 };

// Forward declarations needed before Terminations classes are declared
struct TerminationOptions;
struct BoundarySphereTermOptions;
struct TimeOutTermOptions;

// Forward declaration of Geodesic class needed here; note "Geodesic.h" is NOT included!
class Geodesic;

// Abstract base class for all Terminations
class Termination
{
public:
	Termination() = default;
	virtual ~Termination() = default;

	// Function that is called to determine whether Termination wants to
	// terminate the Geodesic. Returns Term::Continue if no termination wanted,
	// otherwise it returns the appropriate Term condition
	virtual Term CheckTermination(const Geodesic& theGeodesic) = 0;

protected:
	// The termination is itself in charge of keeping track of how many steps it has been since it has been updated
	// The Termination's TerminationOptions struct tells it how many steps it needs to wait between updates
	int m_StepsSinceUpdated{};
};

// The OWNER vector of derived Termination classes
using TerminationUniqueVector = std::vector<std::unique_ptr<Termination>>;

// The Boundary Sphere: this terminates the geodesic (and returns Term::BoundarySphere) if 
// the geodesic reaches outside of the boundary sphere
class BoundarySphereTermination final : public Termination
{
public:
	Term CheckTermination(const Geodesic& theGeodesic) final;

	// The options that the BoundarySphereTermination keeps
	static std::unique_ptr<BoundarySphereTermOptions> TermOptions;

};

// The Time Out: this terminates the geodesic if too many steps have been
// taken in its integration (and returns Term::TimeOut)
class TimeOutTermination final : public Termination
{
public:
	Term CheckTermination(const Geodesic& theGeodesic) final;

	// The options that the TimeOutTermination keeps
	static std::unique_ptr<TimeOutTermOptions> TermOptions;
private:
	// Keep track of the number of steps that the geodesic has taken so far
	int m_CurNrSteps{ 0 };
};


// Base class for TerminationOptions. Other Terminations can inherit from here if they require more options.
struct TerminationOptions
{
public:
	// Basic constructor only sets the number of steps between updates
	TerminationOptions(int Nsteps) : UpdateEveryNSteps{ Nsteps }
	{}

	virtual ~TerminationOptions() = default;

	const int UpdateEveryNSteps;
};

// Options class for BoundarySphere; has to keep track of the BoundarySphere's radius
struct BoundarySphereTermOptions : public TerminationOptions
{
public:
	BoundarySphereTermOptions(real theRadius, int Nsteps) : SphereRadius{theRadius}, TerminationOptions(Nsteps)
	{}

	const real SphereRadius;
};

// Options class for TimeOut; has to keep track of the max. number of integration steps allowed
struct TimeOutTermOptions : public TerminationOptions
{
public:
	TimeOutTermOptions(int MaxStepsAllowed, int Nsteps) : MaxSteps{ MaxStepsAllowed }, TerminationOptions(Nsteps)
	{}

	const int MaxSteps;
};

// Helper to create a new vector of Diagnostic options, based on the bitflag
TerminationUniqueVector CreateTerminationVector(TermBitflag termflags);






#endif
