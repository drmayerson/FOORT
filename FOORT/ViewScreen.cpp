#include "ViewScreen.h" // We are implementing ViewScreen member functions

#include <algorithm> // for std::max

/// <summary>
/// ViewScreen functions
/// </summary>


void ViewScreen::ConstructVielbein()
{
	Point pos{ m_Pos };
	// Adjust r coordinate if using logarithmic coordinate
	pos[1] = m_rLogScale ? log(pos[1]) : pos[1];
	// get metric and inverse metric at our position
	TwoIndex gdd = m_theMetric->getMetric_dd(pos);
	TwoIndex guu = m_theMetric->getMetric_uu(pos);

	// Set the ViewScreen member variable; this is used for setting the initial conditions of each geodesic
	m_Metric_dd = gdd;

	// Helper variable to keep track of the signs of the norms of the vielbein
	OneIndex signs{};

	// helper function to calculate norm of vector
	auto innerprod = [gdd](const OneIndex& vec1, const OneIndex& vec2)->real
	{
		real ret{ 0 };
		for (int i = 0; i < dimension; ++i)
			for (int j = 0; j < dimension; ++j)
				ret += vec1[i] * gdd[i][j] * vec2[j];
		return ret;
	};

	// Construct vielbein
	// First t
	m_Vielbein[0] = guu[0];
	real thenorm = innerprod(m_Vielbein[0], m_Vielbein[0]);
	signs[0] = (thenorm < 0) ? -1 : 1;
	m_Vielbein[0] = signs[0] * m_Vielbein[0] / sqrt(signs[0] * thenorm);

	// Then phi
	m_Vielbein[3] = guu[3] - signs[0] * innerprod(guu[3], m_Vielbein[0]) * m_Vielbein[0];
	thenorm = innerprod(m_Vielbein[3], m_Vielbein[3]);
	signs[3] = (thenorm < 0) ? -1 : 1;
	m_Vielbein[3] = signs[3] * m_Pos[1] * sin(m_Pos[2]) * m_Vielbein[3] / sqrt(signs[3] * thenorm);

	// Then r
	m_Vielbein[1] = guu[1] - signs[0] * innerprod(guu[1], m_Vielbein[0]) * m_Vielbein[0]
		- signs[3] * innerprod(guu[1], m_Vielbein[3]) * m_Vielbein[3];
	thenorm = innerprod(m_Vielbein[1], m_Vielbein[1]);
	signs[1] = (thenorm < 0) ? -1 : 1;
	m_Vielbein[1] = signs[1] * m_Vielbein[1] / sqrt(signs[1] * thenorm);

	// Then theta
	m_Vielbein[2] = guu[2] - signs[0] * innerprod(guu[2], m_Vielbein[0]) * m_Vielbein[0]
		- signs[3] * innerprod(guu[2], m_Vielbein[3]) * m_Vielbein[2];
	-signs[1] * innerprod(guu[2], m_Vielbein[1]) * m_Vielbein[1];
	thenorm = innerprod(m_Vielbein[2], m_Vielbein[2]);
	signs[2] = (thenorm < 0) ? -1 : 1;
	m_Vielbein[2] = signs[2] * m_Pos[1] * m_Vielbein[2] / sqrt(signs[2] * thenorm);

	// Vielbein has been constructed
	// Do some checks
	if (signs[0] * signs[1] * signs[2] * signs[3] > 0)
	{
		ScreenOutput("Vielbein constructed does not seem to have correct negative metric signature!", OutputLevel::Level_0_WARNING);
	}
	if (signs[0] > 0)
	{
		ScreenOutput("Movement along the t coordinate is not timelike; possible ergoregion or horizon.", OutputLevel::Level_0_WARNING);
	}
}

void ViewScreen::SetNewInitialConditions(largecounter index, Point& pos, OneIndex& vel, ScreenIndex& scrIndex) const
{
	// The position of all geodesics is the same: the position of the camera
	pos = m_Pos;
	// Adjust r coordinate if using logarithmic coordinate
	pos[1] = m_rLogScale ? log(pos[1]) : pos[1];

	// Get a new screen point (with (x,y) coordinates between 0 and 1)
	// from the Mesh
	ScreenPoint UnitScreenPos{};
	m_theMesh->getNewInitConds(index, UnitScreenPos, scrIndex);
	// Rescale these into our alpha and beta;
	// alpha runs from screencenter_x-screenwidth/2 to screencenter_y+screenwidth/2 and beta similarly with screenheight
	real alpha = m_ScreenCenter[0] + m_ScreenSize[0] * (UnitScreenPos[0] - 0.5);
	real beta = m_ScreenCenter[1] + m_ScreenSize[1] * (UnitScreenPos[1] - 0.5);
	
	// Note: currently only radially inpointing camera is supported (we do not check m_Direction)

	real sintheta0 = sin(pos[2]);

	// flat frame initial velocity (see FOORT physics documentation)
	real densqrt{ sqrt(m_Pos[1] * m_Pos[1] + alpha * alpha + beta * beta) };
	OneIndex pflat_u{ -1.0, -m_Pos[1] / densqrt, -beta / m_Pos[1] / densqrt, alpha / m_Pos[1] / sintheta0 / densqrt };

	// convert flat frame initial velocity to curved space initial velocity using vielbein
	vel = OneIndex{};
	for (int i = 0; i < dimension; ++i)
		for (int j = 0; j < dimension; ++j)
			vel[i] += m_Vielbein[j][i] * pflat_u[j];

	// rescale velocity so that E = +p_t (+ sign because past-pointing!)
	real energy{ 1.0 }; // energy is arbitrary for null geodesics
	real curenergy{};
	for (int i = 0; i < dimension; ++i)
		curenergy += m_Metric_dd[0][i] * vel[i];
	vel = vel * energy / curenergy;

	// Check to make sure vel is still past-pointing and inward-pointing
	if (vel[0] > 0)
		ScreenOutput("Initial velocity of geodesic is future-pointing (should be past-pointing)!", OutputLevel::Level_0_WARNING);
	if (vel[1] > 0)
		ScreenOutput("Initial velocity of geodesic is outward-pointing (should be inward-pointing)!", OutputLevel::Level_0_WARNING);

	// pos and vel have been set (scrIndex was set above by the call to Mesh already), so we are done!
}

/* OLD (RAPTOR/KERR) IMPLEMENTATION OF INITIAL CONDITIONS
void ViewScreen::SetNewInitialConditionsOLD(largecounter index, Point& pos, OneIndex& vel, ScreenIndex &scrIndex) const
{
	// Helper function for the sign of a real number
	auto sign = [](real arg)->real {return (arg > 0) ? +1 : ((arg < 0) ? -1 : 0); };

	// The position of all geodesics is the same: the position of the camera
	pos = m_Pos;
	// Adjust r coordinate if using logarithmic coordinate
	pos[1] = m_rLogScale ? log(pos[1]) : pos[1];

	// Get a new screen point (with (x,y) coordinates between 0 and 1)
	// from the Mesh
	ScreenPoint UnitScreenPos{};
	m_theMesh->getNewInitConds(index,UnitScreenPos,scrIndex);
	// Rescale these into our alpha and beta;
	// alpha runs from screencenter_x-screenwidth/2 to screencenter_y+screenwidth/2 and beta similarly with screenheight
	real alpha = m_ScreenCenter[0] +  m_ScreenSize[0] * (UnitScreenPos[0] - 0.5);
	real beta = m_ScreenCenter[1] + m_ScreenSize[1] * (UnitScreenPos[1] - 0.5);

	// Note: currently only radially inpointing camera is supported (we do not check m_Direction)

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
	real tempinsqrt{ q - lambda * lambda * costheta0 * costheta0 / sintheta0 / sintheta0 + costheta0 * costheta0 };
	p_down[2] = sign(beta) * Energy * sqrt(std::max(tempinsqrt,0.0));

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

	// pos and vel have been set (scrIndex was set above by the call to Mesh already), so we are done!
}
*/

bool ViewScreen::IsFinished() const
{
	// pass on information to the Mesh
	return m_theMesh->IsFinished();
}

largecounter ViewScreen::getCurNrGeodesics() const
{
	// pass on information to the Mesh
	return m_theMesh->getCurNrGeodesics();
}

void ViewScreen::EndCurrentLoop()
{
	// pass on information to the Mesh
	m_theMesh->EndCurrentLoop();
}

void ViewScreen::GeodesicFinished(largecounter index, std::vector<real> finalValues)
{
	// pass on information to the Mesh
	m_theMesh->GeodesicFinished(index, std::move(finalValues));
}

std::string ViewScreen::getFullDescriptionStr() const
{
	// Full description string; carries over information from the Mesh as well
	return "ViewScreen position: " + toString(m_Pos) + ", screen size: " + toString(m_ScreenSize) + ", "
		+ m_theMesh->getFullDescriptionStr();
}