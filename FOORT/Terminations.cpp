#include"Terminations.h"
#include"Geodesic.h"
#include"Metric.h"

#include<string>
#include<cmath>

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
		theTermVector.emplace_back(new HorizonTermination{theGeodesic});
	}
	// Is BoundarySphere turned on?
	if (termflags & Term_BoundarySphere)
	{
		theTermVector.emplace_back(new BoundarySphereTermination{theGeodesic});
	}
	// Is TimeOut turned on?
	if (termflags & Term_TimeOut)
	{
		theTermVector.emplace_back(new TimeOutTermination{theGeodesic});
	}
	// Is ... turned on?
	// ...
	// (more ifs for other terminations as they are added)

	return theTermVector;
}

bool Termination::DecideUpdate(int UpdateNSteps)
{
	bool decideupdate = false;

	if (UpdateNSteps > 0)
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

Term HorizonTermination::CheckTermination()
{
	Term ret = Term::Continue;

	if (DecideUpdate(TermOptions->UpdateEveryNSteps))
	{
		//ScreenOutput(toString(p),OutputLevel::Level_4_DEBUG);
		//ScreenOutput(std::to_string(m_HorizonRadius), OutputLevel::Level_4_DEBUG);

		// Check to see if radius is almost at horizon; if we are using a logarithmic r scale (u=log(r)) then first exponentiate to get
		// true radius
		real thegeodesicr = (m_theGeodesic->getCurrentPos())[1];
		real r = TermOptions->m_rLogScale ? exp(thegeodesicr) : thegeodesicr;
		if (r < TermOptions->m_HorizonRadius * (1 + TermOptions->m_AtHorizonEps))
		{
			ret = Term::Horizon;
		}
	}

	return ret;
}

std::string HorizonTermination::GetDescriptionString() const
{
	return "Horizon (stop at " + std::to_string(1 + TermOptions->m_AtHorizonEps) + "x(horizon radius))";
}

// Check to see if Boundary Sphere is reached, if so return Term::BoundarySphere
Term BoundarySphereTermination::CheckTermination() 
{
	if (DecideUpdate(TermOptions->UpdateEveryNSteps))
	{
		// Check to see if we reached (past) the boundary sphere (radial coordinate is on position 1 in Point!)
		if ((m_theGeodesic->getCurrentPos())[1] > TermOptions->SphereRadius)
			return Term::BoundarySphere;
		else
			return Term::Continue;
	}
	else // We are not allowed to check the Termination yet
	{
		return Term::Continue;
	}
}

std::string BoundarySphereTermination::GetDescriptionString() const
{
	return "Boundary sphere (R = " + std::to_string(TermOptions->SphereRadius) + ")";
}


// Check to see if enough steps have been taken to time out, if so return Term::TimeOut
Term TimeOutTermination::CheckTermination()
{
	if (DecideUpdate(TermOptions->UpdateEveryNSteps))
	{
		// Check to see if we have done enough steps to time out the integration already
		if (m_CurNrSteps >= TermOptions->MaxSteps)
			return Term::TimeOut;
		else
			return Term::Continue;
	}
	else // We are not allowed to check the Termination yet
	{
		return Term::Continue;
	}
}

std::string TimeOutTermination::GetDescriptionString() const
{
	return "Time out (max integration steps: " + std::to_string(TermOptions->MaxSteps) + ")";
}