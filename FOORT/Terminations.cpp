#include"Terminations.h"
#include"Geodesic.h"

#include<cassert>

// Helper to create a new vector of Terminations, based on the bitflag
TerminationUniqueVector CreateTerminationVector(TermBitflag termflags)
{
	// This should never happen if everything was set up correctly
	assert(termflags != Term_None && "No Terminations in bitflag!");

	TerminationUniqueVector theTermVector{};

	// Is BoundarySphere turned on?
	if (termflags & Term_BoundarySphere)
	{
		theTermVector.emplace_back(new BoundarySphereTermination{});
	}
	// Is TimeOut turned on?
	if (termflags & Term_TimeOut)
	{
		theTermVector.emplace_back(new TimeOutTermination{});
	}
	// Is ... turned on?
	// ...
	// (more ifs for other terminations as they are added)

	return theTermVector;
}

// Check to see if Boundary Sphere is reached, if so return Term::BoundarySphere
Term BoundarySphereTermination::CheckTermination(const Geodesic& theGeodesic) 
{
	// Increment counter of steps since updated. Only check Termination if reached update step
	++m_StepsSinceUpdated;
	if (m_StepsSinceUpdated >= BoundarySphereTermination::TermOptions->UpdateEveryNSteps)
	{
		// Reset step counter
		m_StepsSinceUpdated = 0;

		// Check to see if we reached (past) the boundary sphere (radial coordinate is on position 1 in Point!)
		if ((theGeodesic.getCurrentPos())[1] > BoundarySphereTermination::TermOptions->SphereRadius)
			return Term::BoundarySphere;
		else
			return Term::Continue;
	}
	else // We are not allowed to check the Termination yet
	{
		return Term::Continue;
	}
}


// Check to see if enough steps have been taken to time out, if so return Term::TimeOut
Term TimeOutTermination::CheckTermination(const Geodesic& theGeodesic)
{
	// Increment counter of steps since updated. Only check Termination if reached update step
	++m_StepsSinceUpdated;
	if (m_StepsSinceUpdated >= TimeOutTermination::TermOptions->UpdateEveryNSteps)
	{
		// Reset step counter
		m_StepsSinceUpdated = 0;

		// Increment total number of steps
		++m_CurNrSteps;

		// Check to see if we have done enough steps to time out the integration already
		if (m_CurNrSteps >= TimeOutTermination::TermOptions->MaxSteps)
			return Term::TimeOut;
		else
			return Term::Continue;
	}
	else // We are not allowed to check the Termination yet
	{
		return Term::Continue;
	}
}