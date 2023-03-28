#include "Terminations.h" // We are defining Termination member functions here

#include "Geodesic.h" // // We need member functions of the Geodesic class here
#include "InputOutput.h" // for ScreenOutput()

#include <cmath> // needed for sqrt(), sin(), exp() etc (only on Linux)

/// <summary>
/// Termination helper function
/// </summary>

// Helper to create a new vector of Terminations, based on the bitflag
TerminationUniqueVector CreateTerminationVector(TermBitflag termflags, Geodesic *const theGeodesic)
{
	// This should never happen if everything was set up correctly
	if (termflags == Term_None)
		ScreenOutput("No Terminations in bitflag!", OutputLevel::Level_0_WARNING);

	TerminationUniqueVector theTermVector{};

	// Is Horizon turned on?
	if (termflags & Term_Horizon)
	{
		theTermVector.emplace_back(new HorizonTermination{ theGeodesic });
	}
	// Is BoundarySphere turned on?
	if (termflags & Term_BoundarySphere)
	{
		theTermVector.emplace_back(new BoundarySphereTermination{ theGeodesic });
	}
	// Is TimeOut turned on?
	if (termflags & Term_TimeOut)
	{
		theTermVector.emplace_back(new TimeOutTermination{ theGeodesic });
	}
	// Is ThetaSingularity turned on?
	if (termflags & Term_ThetaSingularity)
	{
		theTermVector.emplace_back(new ThetaSingularityTermination{ theGeodesic });
	}
	// Is NaN turned on?
	if (termflags & Term_NaN)
	{
		theTermVector.emplace_back(new NaNTermination{ theGeodesic });
	}
	// Is GeneralSingularity turned on?
	if (termflags & Term_GeneralSingularity)
	{
		theTermVector.emplace_back(new GeneralSingularityTermination{ theGeodesic });
	}
	//// TERMINATION ADD POINT C ////
	// Add an if statement that checks if your Termination's TermBitflag is turned on, if so add a new instance of it
	// to theTermVector.
	// Sample code:
	/*
	// Is MyTerm turned on?
	if (termflags & Term_MyTerm)
	{
		theTermVector.emplace_back(new MyTermination{ theGeodesic });
	}
	*/
	//// END TERMINATION ADD POINT C ////

	return theTermVector;
}


/// <summary>
/// Termination (abstract base class) functions
/// </summary>


void Termination::Reset()
{
	// Reset to 0 to start integrating new geodesic
	m_StepsSinceUpdated = 0;
}

// This helper function returns true if the Termination should update its internal status. Should be called from within
// CheckTermination() with the appropriate TermOptions::UpdateEveryNSteps
bool Termination::DecideUpdate(largecounter UpdateNSteps)
{
	bool decideupdate = false;

	if (UpdateNSteps == 0) // we always update
		decideupdate = true;
	else // we only update every so many steps
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

/// <summary>
/// HorizonTermination functions
/// </summary>

Term HorizonTermination::CheckTermination()
{
	Term ret = Term::Continue;

	// Check if we are allowed to update (i.e. check for termination)
	if (DecideUpdate(TermOptions->UpdateEveryNSteps))
	{

		// Check to see if radius is almost at horizon;
		// if we are using a logarithmic r scale (u=log(r)) then first exponentiate to get true radius
		real thegeodesicr = (m_OwnerGeodesic->getCurrentPos())[1];
		real r = TermOptions->rLogScale ? exp(thegeodesicr) : thegeodesicr;
		// Check if we are almost at the horizon; the second check is for horizons which are at r=0
		if ( (r < TermOptions->HorizonRadius * (1 + TermOptions->AtHorizonEps))
			|| (TermOptions->HorizonRadius == 0.0 && r < TermOptions->AtHorizonEps) )
		{
			ret = Term::Horizon;
		}
	}

	return ret;
}

std::string HorizonTermination::getFullDescriptionStr() const
{
	// Full description string
	return "Horizon (stop at " + std::to_string(1 + TermOptions->AtHorizonEps) + "x(horizon radius))";
}

/// <summary>
/// BoundarySphereTermination functions
/// </summary>

// Check to see if Boundary Sphere is reached, if so return Term::BoundarySphere
Term BoundarySphereTermination::CheckTermination() 
{
	Term ret = Term::Continue;

	// Check if we are allowed to update (i.e. check for termination)
	if (DecideUpdate(TermOptions->UpdateEveryNSteps))
	{
		// Check to see if we reached (past) the boundary sphere
		// if we are using a logarithmic r scale (u=log(r)) then first exponentiate to get true radius
		real thegeodesicr = (m_OwnerGeodesic->getCurrentPos())[1];
		real r = TermOptions->rLogScale ? exp(thegeodesicr) : thegeodesicr;
		if (r > TermOptions->SphereRadius)
			ret = Term::BoundarySphere;
	}

	return ret;
}

std::string BoundarySphereTermination::getFullDescriptionStr() const
{
	// Full description string
	return "Boundary sphere (R = " + std::to_string(TermOptions->SphereRadius) + ")";
}


/// <summary>
/// TimeOutTermination functions
/// </summary>

void TimeOutTermination::Reset()
{
	// Reset to 0 for new geodesic
	m_CurNrSteps = 0;
	// Call base class implementation to reset base class member variables
	Termination::Reset();
}

// Check to see if enough steps have been taken to time out, if so return Term::TimeOut
Term TimeOutTermination::CheckTermination()
{
	Term ret = Term::Continue;

	if (DecideUpdate(TermOptions->UpdateEveryNSteps))
	{
		// Check to see if we have done enough steps to time out the integration already
		if (m_CurNrSteps >= TermOptions->MaxSteps)
		{
			ret = Term::TimeOut;
		}
		else
		{
			++m_CurNrSteps;
		}
	}

	return ret;
}

std::string TimeOutTermination::getFullDescriptionStr() const
{
	// Full description string
	return "Time out (max integration steps: " + std::to_string(TermOptions->MaxSteps) + ")";
}


/// <summary>
/// ThetaSingularityTermination functions
/// </summary>

// Check to see if enough steps have been taken to time out, if so return Term::TimeOut
Term ThetaSingularityTermination::CheckTermination()
{
	Term ret = Term::Continue;

	if (DecideUpdate(TermOptions->UpdateEveryNSteps))
	{
		// Check to see if theta is too close to a pole
		real theta{ m_OwnerGeodesic->getCurrentPos()[2] };
		real eps{ TermOptions->ThetaSingEpsilon };
		if (fabs(theta) < eps || fabs(pi - theta) < eps)
			ret = Term::ThetaSingularity;
	}

	return ret;
}

std::string ThetaSingularityTermination::getFullDescriptionStr() const
{
	// Full description string
	return "Theta singularity (epsilon: " + std::to_string(TermOptions->ThetaSingEpsilon) + ")";
}

/// <summary>
/// NaNTermination functions
/// </summary>

Term NaNTermination::CheckTermination()
{
	Term ret = Term::Continue;

	if (DecideUpdate(TermOptions->UpdateEveryNSteps))
	{
		// Check to see if nan encountered
		Point pos{ m_OwnerGeodesic->getCurrentPos() };
		OneIndex vel{ m_OwnerGeodesic->getCurrentVel() };

		for (int i = 0; i < dimension && ret == Term::Continue; ++i)
		{
			if (isnan(pos[i]) || isnan(vel[i]))
				ret = Term::NaN;
		}
		if (ret == Term::NaN && TermOptions->OutputToConsole)
		{
			ScreenOutput("NaN encountered for geodesic with screen index " + toString(m_OwnerGeodesic->getScreenIndex())
				+ ", position: " + toString(pos) + ", velocity: " + toString(vel)
				+ ", lambda = " + std::to_string(m_OwnerGeodesic->getCurrentLambda()) + ".",
				OutputLevel::Level_0_WARNING);
		}
	}

	return ret;
}

std::string NaNTermination::getFullDescriptionStr() const
{
	// Full description string
	return "NaN checker ("
		+ std::string(TermOptions->OutputToConsole ? "outputting to console" : "no output to console") + ")";
}


/// <summary>
/// GeneralSingularityTermination functions
/// </summary>

Term GeneralSingularityTermination::CheckTermination()
{
	Term ret = Term::Continue;

	if (DecideUpdate(TermOptions->UpdateEveryNSteps))
	{
		Point pos{ m_OwnerGeodesic->getCurrentPos() };
		// Correct for log r scale if applicable
		pos[1] = TermOptions->rLogScale ? exp(pos[1]) : pos[1];

		for (unsigned int i = 0; i < TermOptions->Singularities.size() && ret == Term::Continue; ++i)
		{
			real distsq{};
			for (int j = 0; j < TermOptions->Singularities[i].size(); ++j)
			{
				distsq += pow(pos[TermOptions->Singularities[i][j].first] - TermOptions->Singularities[i][j].second, 2);
			}
			if (distsq <= TermOptions->Epsilon * TermOptions->Epsilon)
			{
				ret = Term::GeneralSingularity;

				if (TermOptions->OutputToConsole)
				{
					ScreenOutput("Geodesic hitting singularity at " + SingularityToString(i)
						+ " (screen index " + toString(m_OwnerGeodesic->getScreenIndex())
						+ ", position: " + toString(pos) + ", velocity: " + toString(m_OwnerGeodesic->getCurrentVel())
						+ ", lambda = " + std::to_string(m_OwnerGeodesic->getCurrentLambda()) + ").",
						OutputLevel::Level_0_WARNING);
				}
			}
		}
	}

	return ret;
}

std::string GeneralSingularityTermination::SingularityToString(int singnr) const
{
	std::string singstring{ "(" };
	for (int j = 0; j < TermOptions->Singularities[singnr].size(); ++j)
	{
		switch (TermOptions->Singularities[singnr][j].first)
		{
		case 0:
			singstring += 't';
			break;
		case 1:
			singstring += 'r';
			break;
		case 2:
			singstring += "theta";
			break;
		case 3:
			singstring += "phi";
			break;
		}
		singstring += " = " + std::to_string(TermOptions->Singularities[singnr][j].second);
		if (j < TermOptions->Singularities[singnr].size() - 1)
			singstring += ", ";
	}
	singstring += ")";
	return singstring;
}

std::string GeneralSingularityTermination::getFullDescriptionStr() const
{
	// Full description string

	std::string singstring{"{ "};
	for (unsigned int i = 0; i < TermOptions->Singularities.size(); ++i)
	{
		singstring += SingularityToString(i);
		if (i < TermOptions->Singularities.size() - 1)
			singstring += ", ";
	}
	singstring += " }";

	return "Singularities (" + std::to_string(TermOptions->Singularities.size()) +  " singularities at: "
		+ singstring + ", epsilon = " + std::to_string(TermOptions->Epsilon) + ", "
		+ std::string(TermOptions->OutputToConsole ? "outputting to console" : "no output to console") + ")";
}


//// (New Termination classes can define their member functions here)