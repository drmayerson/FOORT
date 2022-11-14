#include"Diagnostics.h"
#include"Metric.h"
#include"Geodesic.h"

#include<memory>
#include<cassert>


// Helper to create a new vector of Diagnostic options, based on the bitflag
DiagnosticUniqueVector CreateDiagnosticVector(DiagBitflag diagflags, DiagBitflag valdiag)
{
	// This should never happen if everything was set up correctly
	assert(diagflags != Diag_None && "No Diagnostics in bitflag!");
	assert(valdiag != Diag_None && "No Diagnostics in bitflag!");

	DiagnosticUniqueVector theDiagVector{};

	// Is FourColorScreen turned on?
	if (diagflags & Diag_FourColorScreen)
	{
		theDiagVector.emplace_back(new FourColorScreenDiagnostic{});
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
		theDiagVector.emplace_back(new GeodesicPositionDiagnostic{});
		// If this is the value Diagnostic, we want it to be the first Diagnostic.
		// Since at the moment it is the last element of the array, we perform a simple rotate right
		// on the current array to place the Diagnostic in the front.
		if (valdiag & Diag_GeodesicPosition)
		{
			std::rotate(theDiagVector.begin(), theDiagVector.begin() + 1, theDiagVector.end());
		}
	}
	// Is ... turned on?
	// ...
	// (more ifs for other diagnostics as they are added)

	return theDiagVector;
}





void FourColorScreenDiagnostic::UpdateData(const Geodesic& theGeodesic)
{
	//if (DecideIfUpdate(theGeodesic))
	//{
		// FourColorScreen only wants to update at the end, and then only if
		// the geodesic has exited the boundary sphere.
		// Check to see that the geodesic has finished, and that in particular
		// it has finished because it has "escaped" to the boundary sphere
		if (theGeodesic.GetTermCondition() == Term::BoundarySphere)
		{
			// Position of the terminated geodesic
			Point pos{ theGeodesic.getCurrentPos() };
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

			// We updated the diagnostic data, so reset this counter
			m_StepsSinceUpdated = 0;
		}
		else
		{
			// We will not update at this time; increment the number of steps that has passed since update
			++m_StepsSinceUpdated;
		}
	//}
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


void GeodesicPositionDiagnostic::UpdateData(const Geodesic& theGeodesic)
{
	if (DecideIfUpdate(theGeodesic))
	{
		// Put the current position in the saved position vector
		m_AllSavedPoints.push_back(theGeodesic.getCurrentPos());
	}

	// if geodesic is done integrating. Check if we need to resize all the saved points
	if (theGeodesic.GetTermCondition() != Term::Continue)
	{
		
		int nrstepstokeep{ GeodesicPositionDiagnostic::DiagOptions->OutputNrSteps };

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
		outputstr += "; ";
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
	assert(val1.size() == 2 && val2.size() == 2 && "Wrong values given to GeodesicPositionDiagnostic::FinalDataValDistance!");

	ScreenOutput("This needs updating!", OutputLevel::Level_4_DEBUG);
	return abs((val1[0] - val1[0]) * (val1[0] - val1[0]) + (val1[1] - val1[1]) * (val1[1] - val1[1]));
}

bool GeodesicPositionDiagnostic::DecideIfUpdate(const Geodesic& theGeodesic)
{
	bool decideupdate = false;

	// Check if it is start and we (only) update at start (and/or finish)
	if ((GeodesicPositionDiagnostic::DiagOptions->UpdateEveryNSteps == Update_OnlyStart
		|| GeodesicPositionDiagnostic::DiagOptions->UpdateEveryNSteps == Update_OnlyStartAndFinish)
		&& theGeodesic.getCurrentLambda() == 0.0)
	{
		decideupdate = true;
	}
	// Check if it is finish and we (only) update at finish (and/or start)
	else if ((GeodesicPositionDiagnostic::DiagOptions->UpdateEveryNSteps == Update_OnlyFinish
		|| GeodesicPositionDiagnostic::DiagOptions->UpdateEveryNSteps == Update_OnlyStartAndFinish)
		&& theGeodesic.GetTermCondition() != Term::Continue)
	{
		decideupdate = true;
	}
	// Check if we update every n steps
	else if (GeodesicPositionDiagnostic::DiagOptions->UpdateEveryNSteps > 0)
	{
		// increase step counter and check if it is time to update
		++m_StepsSinceUpdated;
		if (m_StepsSinceUpdated >= GeodesicPositionDiagnostic::DiagOptions->UpdateEveryNSteps)
		{
			// Time to update and reset step counter
			decideupdate = true;
			m_StepsSinceUpdated = 0;
		}
	}
	return decideupdate;
}