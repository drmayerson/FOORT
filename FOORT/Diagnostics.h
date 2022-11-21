#ifndef _FOORT_DIAGNOSTICS_H
#define _FOORT_DIAGNOSTICS_H

#include<memory>
#include<vector>
#include<string>
#include<bitset>

#include"Geometry.h"
// #include "InputOutput.h"

// Diagnostic bitflags
// Used for constructing vector of Diagnostics
// Note that this means every diagnostic is either "on" or "off";
// it is not possible to have a Diagnostic "on" more than once
using DiagBitflag = std::uint16_t;

constexpr DiagBitflag Diag_None					{ 0b0000'0000'0000'0000 };
constexpr DiagBitflag Diag_GeodesicPosition		{ 0b0000'0000'0000'0001 };
constexpr DiagBitflag Diag_FourColorScreen		{ 0b0000'0000'0000'0010 };
constexpr DiagBitflag Diag_EquatorialPasses 	{ 0b0000'0000'0000'0100 };
constexpr DiagBitflag Diag_PassThroughSurface	{ 0b0000'0000'0000'1000 };

// All settings of UpdateEveryNSteps > 0 mean to update every so many steps;
// setting < 0 mean only to update at start, finish, or both
constexpr int Update_OnlyStart = -1;
constexpr int Update_OnlyFinish = -2;
constexpr int Update_OnlyStartAndFinish = -3;

// Forward declaration needed before Diagnostic
struct DiagnosticOptions;
struct GeodesicPositionOptions;

// Forward declaration of Geodesic class needed here; note "Geodesic.h" is NOT included!
class Geodesic;

// Abstract base class for all diagnostics
class Diagnostic
{
public:
	struct BasicOptions
	{
		const int UpdateEveryNSteps;
	};

	Diagnostic(Geodesic* const theGeodesic) : m_theGeodesic{ theGeodesic }
	{}

	virtual ~Diagnostic() = default;

	virtual void UpdateData() = 0;

	// These functions are for use at the end of integration of a geodesic.
	// getFullData() returns all the data stored in the Diagnostic as a string (for output to file)
	virtual std::string getFullData() const = 0;
	// getFinalDataVal() associates a value to the geodesic corresponding to the final value of this diagnostic
	// (used for determining coarseness of nearby geodesics)
	virtual std::vector<real> getFinalDataVal() const = 0;

	// Function used to determine distance between two values obtained from getFinalDataVal()
	// (for determining coarseness of nearby geodesics)
	virtual real FinalDataValDistance(const std::vector<real>& val1, const std::vector<real>& val2) const = 0;

	virtual std::string GetDiagNameStr() const = 0;
	virtual std::string GetDescriptionString() const;
protected:
	// The geodesic that the Diagnostic is watching; a const pointer to the Geodesic
	Geodesic* const m_theGeodesic;

	bool DecideUpdate(int UpdateNSteps);

	// The diagnostic is itself in charge of keeping track of how many steps it has been since it has been updated
	// The Diagnostic's DiagnosticOptions struct tells it how many steps it needs to wait between updates
	int m_StepsSinceUpdated{};
};


// Owner vector of derived Diagnostics classes
using DiagnosticUniqueVector = std::vector<std::unique_ptr<Diagnostic>>;


// The four color screen: associates one of four colors based on the quadrant that the geodesic finishes in
// Will ONLY return a color if the geodesic indeed finishes because it passes through the boundary sphere!
class FourColorScreenDiagnostic final : public Diagnostic
{
public:
	FourColorScreenDiagnostic(Geodesic* const theGeodesic) : Diagnostic(theGeodesic) {}

	// Both of these output functions simply returns the quadrant number associated with the geodesic's end position
	std::string getFullData() const override;
	std::vector<real> getFinalDataVal() const override;

	// Discrete metric for distance: returns 0 if the quadrants are the same, 1 if they are not
	real FinalDataValDistance(const std::vector<real>& val1, const std::vector<real>& val2) const override;

	// Static member of this class that contains its DiagnosticOptions struct.
	// FourColorScreen only needs the base class options
	static std::unique_ptr<DiagnosticOptions> DiagOptions;

	std::string GetDiagNameStr() const override;
	std::string GetDescriptionString() const override;
protected:
	void UpdateData() override;

	// Note initialization to 0; this means the default value returned will be 0
	// (e.g. if Term::BoundarySphere is not reached)
	int m_quadrant{0};
};

// Geodesic position tracker
class GeodesicPositionDiagnostic final : public Diagnostic
{
public:
	GeodesicPositionDiagnostic(Geodesic* const theGeodesic) : Diagnostic(theGeodesic) {}

	std::string getFullData() const override;
	std::vector<real> getFinalDataVal() const override;

	real FinalDataValDistance(const std::vector<real>& val1, const std::vector<real>& val2) const override;

	static std::unique_ptr<GeodesicPositionOptions> DiagOptions;

	std::string GetDiagNameStr() const override;
	std::string GetDescriptionString() const override;

protected:
	void UpdateData() override;

	std::vector<Point> m_AllSavedPoints{};
};


// Counting number of passes through equatorial plane
class EquatorialPassesDiagnostic final : public Diagnostic
{
public:
	EquatorialPassesDiagnostic(Geodesic* const theGeodesic) : Diagnostic(theGeodesic) {}

	std::string getFullData() const override;
	std::vector<real> getFinalDataVal() const override;

	real FinalDataValDistance(const std::vector<real>& val1, const std::vector<real>& val2) const override;

	static std::unique_ptr<DiagnosticOptions> DiagOptions;

	std::string GetDiagNameStr() const override;
	std::string GetDescriptionString() const override;

protected:
	void UpdateData() override;

	int m_EquatPasses{ 0 };

	real m_PrevTheta{ -1 };
};

// Base class for DiagnosticOptions. Other Diagnostics can inherit from here if they require more options.
struct DiagnosticOptions
{
public:
	// Basic constructor only sets the number of steps between updates
	DiagnosticOptions(int Nsteps) : UpdateEveryNSteps{ Nsteps }
	{}

	virtual ~DiagnosticOptions() = default;

	const int UpdateEveryNSteps;
};

struct GeodesicPositionOptions : public DiagnosticOptions
{
public:
	// Basic constructor only sets the number of steps between updates
	GeodesicPositionOptions(int outputsteps, int Nsteps) : OutputNrSteps{ outputsteps }, DiagnosticOptions(Nsteps)
	{}

	const int OutputNrSteps;
};

// Helper to create a new vector of Diagnostic options, based on the bitflag
// The first diagnostic is the value diagnostic
DiagnosticUniqueVector CreateDiagnosticVector(DiagBitflag diagflags, DiagBitflag valdiag, Geodesic *const theGeodesic);


#endif
