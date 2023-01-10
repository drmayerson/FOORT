#include "Diagnostics.h" // We are defining Diagnostic member functions here

#include "Geodesic.h" // We need member functions of the Geodesic class here
#include "InputOutput.h" // for ScreenOutput()

#include <algorithm> // needed for std::rotate()
#include <cmath> // needed for cos, sin, acos

/// <summary>
/// Diagnostic helper function
/// </summary>

// Helper to create a new vector of Diagnostic options, based on the bitflag
DiagnosticUniqueVector CreateDiagnosticVector(DiagBitflag diagflags, DiagBitflag valdiag, Geodesic *const theGeodesic)
{
	// This should never happen if everything was set up correctly
	if(diagflags == Diag_None)
		ScreenOutput("No Diagnostics in bitflag!", OutputLevel::Level_0_WARNING);
	if (valdiag == Diag_None)
		ScreenOutput("No Diagnostics in bitflag!", OutputLevel::Level_0_WARNING);

	DiagnosticUniqueVector theDiagVector{};

	// We will now determine for each Diagnostic individually whether it is turned on in diagflags,
	// and if so add an instance of it to theDiagVector. If it is additionally also the valdiag,
	// then we move it to the front of the vector.

	// Is FourColorScreen turned on?
	if (diagflags & Diag_FourColorScreen)
	{
		theDiagVector.emplace_back(new FourColorScreenDiagnostic{ theGeodesic });
		// If this is the value Diagnostic, we want it to be the first Diagnostic.
		// Since at the moment it is the last element of the array, we perform a simple rotate right
		// on the current array to place the Diagnostic in the front.
		if (valdiag & Diag_FourColorScreen)
		{
			std::rotate(theDiagVector.begin(), theDiagVector.begin() + 1, theDiagVector.end());
		}
	}
	// Is GeodesicPosition turned on?
	if (diagflags & Diag_GeodesicPosition)
	{
		theDiagVector.emplace_back(new GeodesicPositionDiagnostic{ theGeodesic });
		// If this is the value Diagnostic, we want it to be the first Diagnostic.
		// Since at the moment it is the last element of the array, we perform a simple rotate right
		// on the current array to place the Diagnostic in the front.
		if (valdiag & Diag_GeodesicPosition)
		{
			std::rotate(theDiagVector.begin(), theDiagVector.begin() + 1, theDiagVector.end());
		}
	}
	// Is EquatorialPasses turned on?
	if (diagflags & Diag_EquatorialPasses)
	{
		theDiagVector.emplace_back(new EquatorialPassesDiagnostic{ theGeodesic });
		// If this is the value Diagnostic, we want it to be the first Diagnostic.
		// Since at the moment it is the last element of the array, we perform a simple rotate right
		// on the current array to place the Diagnostic in the front.
		if (valdiag & Diag_EquatorialPasses)
		{
			std::rotate(theDiagVector.begin(), theDiagVector.begin() + 1, theDiagVector.end());
		}
	}

	//// GEODESIC ADD POINT C ////
	// Add an if statement that checks if your Diagnostic's DiagBitflag is turned on, if so add a new instance of it
	// to theDiagVector. Then check if valdiag is your Diagnostic and rotate theDiagVector accordingly if it is.
	// Sample code:
	/*
	// Is MyDiagnostic turned on?
	if (diagflags & Diag_MyDiag)
	{
		theDiagVector.emplace_back(new MyDiagnostic{ theGeodesic });
		// If this is the value Diagnostic, we want it to be the first Diagnostic.
		// Since at the moment it is the last element of the array, we perform a simple rotate right
		// on the current array to place the Diagnostic in the front.
		if (valdiag & Diag_MyDiag)
		{
			std::rotate(theDiagVector.begin(), theDiagVector.begin() + 1, theDiagVector.end());
		}
	}
	*/
	//// END GEODESIC ADD POINT C ////


	return theDiagVector;
}

/// <summary>
/// Diagnostic (abstract base class) functions
/// </summary>

void Diagnostic::Reset()
{
	// Reset steps to 0 for starting a new geodesic
	m_StepsSinceUpdated = 0;
}

// This helper function returns true if the Diagnostic should update its internal status. Should be called from within
// UpdateData() with the appropriate DiagnosticOptions::theUpdateFrequency
bool Diagnostic::DecideUpdate(const UpdateFrequency& myUpdateFrequency)
{
	bool decideupdate = false;
	// First, check if we are to update every so many steps
	if (myUpdateFrequency.UpdateNSteps > 0)
	{
		// increase step counter and check if it is time to update
		++m_StepsSinceUpdated;
		if (m_StepsSinceUpdated >= myUpdateFrequency.UpdateNSteps)
		{
			// Time to update and reset step counter
			decideupdate = true;
			m_StepsSinceUpdated = 0;
		}
	}
	// If UpdateNsteps == 0, then we only update either at start and/or end of the integration, so check these
	// cases
	else if (myUpdateFrequency.UpdateStart && m_OwnerGeodesic->getCurrentLambda() == 0.0)
	{
		decideupdate = true;
	}
	else if (myUpdateFrequency.UpdateFinish && m_OwnerGeodesic->getTermCondition() != Term::Continue)
	{
		decideupdate = true;
	}
	
	return decideupdate;
}

// Base class definition just returns the short name (which itself is pure virtual in the base class!)
std::string Diagnostic::getFullDescriptionStr() const
{
	return getNameStr();
}

/// <summary>
/// FourColorScreen functions
/// </summary>

void FourColorScreenDiagnostic::Reset()
{
	// Reset the quadrant to its default option; also call base class Reset function
	m_quadrant = 0;
	Diagnostic::Reset();
}

void FourColorScreenDiagnostic::UpdateData()
{
	// Note: FourColorScreen only wants to update at the end, and then only if
	// the geodesic has exited the boundary sphere.
	// Check to see that the geodesic has finished, and that in particular
	// it has finished because it has "escaped" to the boundary sphere.
	if (m_OwnerGeodesic->getTermCondition() == Term::BoundarySphere)
	{
		// Position of the terminated geodesic
		Point pos{ m_OwnerGeodesic->getCurrentPos() };
		// Rework phi coordinate to be between 0 and 2pi
		while (pos[3] > 2 * pi)
			pos[3] -= 2 * pi;
		while (pos[3] < 0)
			pos[3] += 2 * pi;

		// Check which quadrant the geodesic is in
		int quadrant{ 0 };
		if (pos[2] < pi / 2)
		{
			if (pos[3] < pi)
				quadrant = 1;
			else
				quadrant = 2;
		}
		else
		{
			if (pos[3] < pi)
				quadrant = 3;
			else
				quadrant = 4;
		}

		// Put this quadrant data into the class' quadrant data
		m_quadrant = quadrant;
	}
}

std::string FourColorScreenDiagnostic::getFullDataStr() const
{
	// Return the value of the quadrant as string
	return std::to_string(m_quadrant);
}

std::vector<real> FourColorScreenDiagnostic::getFinalDataVal() const
{
	// Return the quadrant as a vector (of size one)
	return std::vector<real> {static_cast<real>(m_quadrant)};
}

real FourColorScreenDiagnostic::FinalDataValDistance(const std::vector<real>& val1, const std::vector<real>& val2) const
{
	// Discrete metric for distance: returns 0 if the quadrants are the same, 1 if they are not
	if (abs(val1[0] - val2[0]) < 1) // use <1 instead of == 0.0 to avoid floating point round-off errors
		return 0;
	else
		return 1;
}

std::string FourColorScreenDiagnostic::getNameStr() const
{
	// Simple name without spaces
	return "FourColorScreen";
}

std::string FourColorScreenDiagnostic::getFullDescriptionStr() const
{
	// Full description does not have more information than simple name, but can contain spaces
	return "Four-color screen";
}

/// <summary>
/// GeodesicPositionDiagnostic functions
/// </summary>

void GeodesicPositionDiagnostic::Reset()
{
	// Empty out vector of points; also call base class Reset function
	m_AllSavedPoints.clear();
	Diagnostic::Reset();
}

void GeodesicPositionDiagnostic::UpdateData()
{
	// This checks to see if we want to update the data now (and increments the step counter if necessary)
	if (DecideUpdate(DiagOptions->theUpdateFrequency))
	{
		// Put the current position in the saved position vector
		m_AllSavedPoints.push_back(m_OwnerGeodesic->getCurrentPos());
	}


	// We also want extra behaviour at the end, when the geodesic is done integrating:
	// at this point, we want to resize the vector of saved points if needed.
	if (m_OwnerGeodesic->getTermCondition() != Term::Continue) // we are done integrating
	{
		largecounter nrstepstokeep{ DiagOptions->OutputNrSteps };

		// check if we need to resize; note that nrstepstokeep == 0 if we keep all of the steps
		if (nrstepstokeep > 0 && nrstepstokeep < m_AllSavedPoints.size())
		{
			largecounter jettison = static_cast<largecounter>(m_AllSavedPoints.size()) / nrstepstokeep;
			// we create a temporary vector that stores all the data we are keeping
			std::vector<Point> tmp{};
			for (largecounter i = 0; i < m_AllSavedPoints.size(); ++i)
			{
				if (i % jettison == 0)
					tmp.push_back(m_AllSavedPoints[i]);
			}
			// Make sure to keep last step
			if ((m_AllSavedPoints.size() - 1) % jettison != 0) // if we have not already saved the last step
			{
				// delete the last saved step and replace it by the actual last entry of the data
				tmp.pop_back();
				tmp.push_back(m_AllSavedPoints[m_AllSavedPoints.size() - 1]);
			}

			m_AllSavedPoints = tmp;
		}
	}
}

std::string GeodesicPositionDiagnostic::getFullDataStr() const
{
	// The full output string looks like this:
	// "(total nr steps) ;; (step 1) (step 2) (step 3) ..."
	// where each step is a (space-separated) output of the geodesic coordinates at that step
	std::string outputstr{ std::to_string(m_AllSavedPoints.size()) + " ;; "};

	for (auto& output : m_AllSavedPoints)
	{
		// We are not using the toString() function since that contains extraneous brackets and commas that
		// we don't want in our output
		for (int i = 0; i < dimension; ++i)
			outputstr += std::to_string(output[i]) + " ";
	}

	return outputstr;
}

std::vector<real> GeodesicPositionDiagnostic::getFinalDataVal() const
{
	// return the last (theta, phi) coordinates
	Point lastpt{ m_AllSavedPoints.back() };
	return std::vector<real>{lastpt[2],lastpt[3]};
}

real GeodesicPositionDiagnostic::FinalDataValDistance(const std::vector<real>& val1, const std::vector<real>& val2) const
{
	// Check to make sure we have the right size vectors passed
	if (val1.size() != 2 || val2.size() != 2)
	{
		ScreenOutput("Wrong values given to GeodesicPositionDiagnostic::FinalDataValDistance!", OutputLevel::Level_0_WARNING);
		return 0;
	}
	
	// Return the arc length between these two points
	return acos( cos(val1[0]) * cos(val2[0]) + sin(val1[0]) * sin(val2[0]) * cos(val1[1] - val2[1]) );
}

std::string GeodesicPositionDiagnostic::getNameStr() const
{
	// Simple name string without spaces
	return  "GeodesicPosition" ;
}

std::string GeodesicPositionDiagnostic::getFullDescriptionStr() const
{
	// Full description string; also contains information about how frequently it updates and how many steps it outputs at the end
	return "Geodesic position (output " + std::to_string(DiagOptions->OutputNrSteps) + 
		" steps, updates every " + std::to_string(DiagOptions->theUpdateFrequency.UpdateNSteps) + " steps)";
}


/// <summary>
/// EquatorialPassesDiagnostic functions
/// </summary>

void EquatorialPassesDiagnostic::Reset()
{
	// Reset internal variables to default (initial) values; also call base class Reset function
	m_EquatPasses = 0;
	m_PrevTheta = -1;
	Diagnostic::Reset();
}


void EquatorialPassesDiagnostic::UpdateData()
{
	// This checks to see if we want to update the data now (and increments the step counter if necessary)
	if (DecideUpdate(DiagOptions->theUpdateFrequency))
	{
		// Get the current theta coordinate of the geodesic
		real curTheta{ m_OwnerGeodesic->getCurrentPos()[2] };

		// We only do anything (check for a pass and/or update the past theta) if the geodesic has
		// passed over a threshold around the equatorial plane
		if ( abs(curTheta - pi / 2.0) > pi / 2.0 * DiagOptions->Threshold )
		{
			// This checks to see if we have crossed the equatorial plane by comparing the previous theta coordinate
			// with the current theta coordinate
			// (Note that m_PrevTheta = -1 at initialization)
			if (m_PrevTheta > 0 && ((m_PrevTheta - pi / 2.0) * (curTheta - pi / 2.0)) < 0.0)
				++m_EquatPasses;

			// The current theta coordinate becomes the previous coordinate for the next iteration
			m_PrevTheta = curTheta;
		}
	}

	// If the geodesic is finished integrating and we finish inside the horizon, make the passes negative
	// (to have a difference between inside and outside the horizon)
	if (m_OwnerGeodesic->getTermCondition() == Term::Horizon)
	{
		m_EquatPasses = -m_EquatPasses;
	}
}


std::string EquatorialPassesDiagnostic::getFullDataStr() const
{
	// Returns a string of how many times it passed across the equatorial plane
	return std::to_string(m_EquatPasses);
}

std::vector<real> EquatorialPassesDiagnostic::getFinalDataVal() const
{
	// Simple vector of size one containing the number of equatorial passes
	return std::vector<real> {static_cast<real>(m_EquatPasses)};
}

real EquatorialPassesDiagnostic::FinalDataValDistance(const std::vector<real>& val1, const std::vector<real>& val2) const
{
	// Returns the simple distance between two geodesics. Note that
	// a geodesic terminating inside the horizon has NEGATIVE number of equatorial passes, so a geodesic inside and outside
	// the horizon will always have non-zero distance.
	return abs(val1[0] - val2[0]);
}

std::string EquatorialPassesDiagnostic::getNameStr() const
{
	// Simple name string without spaces
	return  "EquatPasses";
}

std::string EquatorialPassesDiagnostic::getFullDescriptionStr() const
{
	// More descriptive string (with spaces)
	return "Equatorial passes (threshold = " + std::to_string(DiagOptions->Threshold) + ")";
}


//// (New Diagnostic classes can define their member functions here)


