#include"Diagnostics.h"
#include"Metric.h"
#include"Geodesic.h"
#include "InputOutput.h"

#include<memory>


// Helper to create a new vector of Diagnostic options, based on the bitflag
DiagnosticUniqueVector CreateDiagnosticVector(DiagBitflag diagflags, DiagBitflag valdiag, Geodesic *const theGeodesic)
{
	// This should never happen if everything was set up correctly
	if(diagflags == Diag_None)
		ScreenOutput("No Diagnostics in bitflag!", OutputLevel::Level_0_WARNING);
	if (diagflags == Diag_None)
		ScreenOutput("No Diagnostics in bitflag!", OutputLevel::Level_0_WARNING);

	DiagnosticUniqueVector theDiagVector{};

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
	// Is ... turned on?
	// ...
	// (more ifs for other diagnostics as they are added)

	return theDiagVector;
}


bool Diagnostic::DecideUpdate(int UpdateNSteps)
{
	bool decideupdate = false;
	
	// Check if it is start and we (only) update at start (and/or finish)
	if ((UpdateNSteps == Update_OnlyStart
		|| UpdateNSteps == Update_OnlyStartAndFinish)
		&& m_theGeodesic->getCurrentLambda() == 0.0)
	{
		decideupdate = true;
	}
	// Check if it is finish and we (only) update at finish (and/or start)
	else if ((UpdateNSteps == Update_OnlyFinish
		|| UpdateNSteps == Update_OnlyStartAndFinish)
		&& m_theGeodesic->GetTermCondition() != Term::Continue)
	{
		decideupdate = true;
	}
	// Check if we update every n steps
	else if (UpdateNSteps > 0)
	{
		// increase step counter and check if it is time to update
		++m_StepsSinceUpdated;
		if (m_StepsSinceUpdated >= UpdateNSteps)
		{
			// Time to update and reset step counter
			decideupdate = true;
			m_StepsSinceUpdated = 0;
		}
	}
	return decideupdate;
}

std::string Diagnostic::GetDescriptionString() const
{
	return GetDiagNameStr();
}

void FourColorScreenDiagnostic::UpdateData()
{
	// This checks to see if we want to update the data now (and increments the step counter if necessary)
	if (DecideUpdate(DiagOptions->UpdateEveryNSteps))
	{
		// Note: FourColorScreen only wants to update at the end, and then only if
		// the geodesic has exited the boundary sphere.
		// Check to see that the geodesic has finished, and that in particular
		// it has finished because it has "escaped" to the boundary sphere.
		if (m_theGeodesic->GetTermCondition() == Term::BoundarySphere)
		{
			// Position of the terminated geodesic
			Point pos{ m_theGeodesic->getCurrentPos() };
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
}

std::string FourColorScreenDiagnostic::getFullData() const
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
	if (abs(val1[0] - val2[0])<1)
		return 0;
	else
		return 1;
}

std::string FourColorScreenDiagnostic::GetDiagNameStr() const
{
	return std::string{ "FourColorScreen" };
}

std::string FourColorScreenDiagnostic::GetDescriptionString() const
{
	return "Four-color screen";
}


void GeodesicPositionDiagnostic::UpdateData()
{
	// This checks to see if we want to update the data now (and increments the step counter if necessary)
	if (DecideUpdate(DiagOptions->UpdateEveryNSteps))
	{
		// Put the current position in the saved position vector
		m_AllSavedPoints.push_back(m_theGeodesic->getCurrentPos());
	}

	// if geodesic is done integrating. Check if we need to resize all the saved points
	if (m_theGeodesic->GetTermCondition() != Term::Continue)
	{
		
		int nrstepstokeep{ DiagOptions->OutputNrSteps };

		// check if we need to resize
		if (nrstepstokeep > 0 && nrstepstokeep < m_AllSavedPoints.size())
		{
			int jettison = m_AllSavedPoints.size() / nrstepstokeep;
			std::vector<Point> tmp{};
			for (int i = 0; i < m_AllSavedPoints.size(); ++i)
			{
				if (i % jettison == 0)
					tmp.push_back(m_AllSavedPoints[i]);
			}
			// Make sure to keep last step
			if ((m_AllSavedPoints.size() - 1) % jettison != 0)
			{
				tmp.pop_back();
				tmp.push_back(m_AllSavedPoints[m_AllSavedPoints.size() - 1]);
			}

			m_AllSavedPoints = tmp;
		}
	}
}

std::string GeodesicPositionDiagnostic::getFullData() const
{
	std::string outputstr{ std::to_string(m_AllSavedPoints.size()) + " ;; "};

	for (auto& output : m_AllSavedPoints)
	{
		for (int i = 0; i < dimension; ++i)
			outputstr += std::to_string(output[i]) + " ";
		//outputstr += "; ";
	}

	return outputstr;
}

std::vector<real> GeodesicPositionDiagnostic::getFinalDataVal() const
{
	// return the last (theta, phi)
	Point lastpt{ m_AllSavedPoints.back() };
	return std::vector<real>{lastpt[2],lastpt[3]};
}

real GeodesicPositionDiagnostic::FinalDataValDistance(const std::vector<real>& val1, const std::vector<real>& val2) const
{
	if (val1.size() != 2 || val2.size() != 2)
	{
		ScreenOutput("Wrong values given to GeodesicPositionDiagnostic::FinalDataValDistance!", OutputLevel::Level_0_WARNING);
		return 0;
	}

	ScreenOutput("Using GeodesicPositionDiagnostic::FinalDataValDistance without implementation!", OutputLevel::Level_0_WARNING);
	return abs((val1[0] - val1[0]) * (val1[0] - val1[0]) + (val1[1] - val1[1]) * (val1[1] - val1[1]));
}

std::string GeodesicPositionDiagnostic::GetDiagNameStr() const
{
	return std::string{ "GeodesicPosition" };
}

std::string GeodesicPositionDiagnostic::GetDescriptionString() const
{
	return "Geodesic position (output " + std::to_string(DiagOptions->OutputNrSteps) + 
		" steps, updates every " + std::to_string(DiagOptions->UpdateEveryNSteps) + " steps)";
}


std::string EquatorialPassesDiagnostic::getFullData() const
{
	return std::to_string(m_EquatPasses);
}

std::vector<real> EquatorialPassesDiagnostic::getFinalDataVal() const
{
	return std::vector<real> {static_cast<real>(m_EquatPasses)};
}

real EquatorialPassesDiagnostic::FinalDataValDistance(const std::vector<real>& val1, const std::vector<real>& val2) const
{
	return abs(val1[0] - val2[0]);
}

void EquatorialPassesDiagnostic::UpdateData()
{
	if (DecideUpdate(DiagOptions->UpdateEveryNSteps))
	{
		real curTheta{ m_theGeodesic->getCurrentPos()[2] };

		if (m_PrevTheta > 0 && ( (m_PrevTheta-pi/2.0) * (curTheta-pi/2.0) ) < 0.0)
			++m_EquatPasses;

		m_PrevTheta = curTheta;
	}
}

std::string EquatorialPassesDiagnostic::GetDiagNameStr() const
{
	return std::string{ "EquatPasses" };
}

std::string EquatorialPassesDiagnostic::GetDescriptionString() const
{
	return "Equatorial passes";
}


