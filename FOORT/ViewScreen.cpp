#include"ViewScreen.h"

void ViewScreen::SetNewInitialConditions(int index, Point& pos, OneIndex& vel, ScreenIndex &scrIndex)
{
	assert(m_GeodType == GeodesicType::Null && "Only null geodesics supported at the moment!");

	// Helper function for the sign of a real number
	auto sign = [](real arg)->real {return (arg > 0) ? +1 : ((arg < 0) ? -1 : 0); };

	// The position of all geodesics is the same: the position of the camera
	pos = m_Pos;

	// Get a new screen point (with (x,y) coordinates between 0 and 1)
	// from the Mesh
	ScreenPoint UnitScreenPos{};
	m_theMesh->getNewInitConds(index,UnitScreenPos,scrIndex);
	// Rescale these into our alpha and beta;
	// alpha runs from -screenwidth/2 to screenwidth/2 and beta similarly with screenheight
	real alpha = m_ScreenSize[0] * (UnitScreenPos[0] - 0.5);
	real beta = m_ScreenSize[1] * (UnitScreenPos[1] - 0.5);

	// Currently only radially inpointing camera is supported; this should have been overridden at initialization
	assert(m_Direction == OneIndex{ 0,-1,0,0 });

	real costheta0 = cos(pos[2]);
	real sintheta0 = sin(pos[2]);
	
	// We use the expressions of [CB]: Cunningham & Bardeen 1973 "Star Orbiting extreme Kerr Black Hole" 
	// NOTE: These expressions are only valid asymptotically, i.e. for large distance from the orbit.
	// It is a possibility to provide a more general setup valid anywhere (based on a local orthonormal frame)
	// We also assume the axis of rotation (if any) is the z-axis

	// Conserved quantities (asymptotically, in Kerr metric notation) are the
	// Energy E = -p_t, azimuthal angular momentum L_z = p_\phi,
	// and Carter constant Q.
	// We then define q = Q/E^2 and lambda = L_z/E.
	
	// [CB]	(28) inverted to give q and lambda (note that (28b) has a typo -- it should be q instead of q^2) is
	// q = beta^2 + (alpha^2-1)cos^2\theta
	real q = beta * beta + (alpha * alpha - 1) * costheta0 * costheta0;
	// and lambda = - alpha\sin\theta
	real lambda = -alpha * sintheta0;

	// Set the energy scale; for photons this is irrelevant
	real Energy = 1;

	// [CB] (13), (14), (15) give the covariant components p_t, p_phi , p_theta, (p_t and p_phi see above)
	// p_theta = sgn(beta) sqrt(Q-L^2\cot^2\theta + E^2\cos^2\theta)
	OneIndex p_down{ 0,0,0,0 };
	p_down[0] = -Energy;
	p_down[3] = lambda * Energy;
	p_down[2] = sign(beta) * Energy * sqrt(q
		- lambda * lambda * costheta0 * costheta0 / sintheta0 / sintheta0
		+ costheta0 * costheta0);

	// Now, use the metric at the geodesic position to raise the vector and get the geodesic's initial velocity
	TwoIndex metricpos_uu = m_theMetric->getMetric_uu(pos);
	vel = OneIndex{ 0,0,0,0 };
	for (int i = 0; i < dimension; ++i)
	{
		vel[0] += metricpos_uu[0][i] * p_down[i];
		vel[2] += metricpos_uu[2][i] * p_down[i];
		vel[3] += metricpos_uu[3][i] * p_down[i];
	}
	// The final entry is determined by demanding that the geodesic is null
	// sign of velocity is determined by having geodesic point inwards
	vel[1] = -sqrt(-metricpos_uu[1][1] * (vel[0] * p_down[0] + vel[2] * p_down[2] + vel[3] * p_down[3]));

	// pos and vel have been set, so we are done!
}

bool ViewScreen::IsFinished()
{
	return m_theMesh->IsFinished();
}

int ViewScreen::getCurNrGeodesics()
{
	return m_theMesh->getCurNrGeodesics();
}

void ViewScreen::EndCurrentLoop()
{
	m_theMesh->EndCurrentLoop();
}

void ViewScreen::GeodesicFinished(int index, std::vector<real> finalValues)
{
	m_theMesh->GeodesicFinished(index, finalValues);
}