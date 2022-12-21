#ifndef _FOORT_VIEWSCREEN_H
#define _FOORT_VIEWSCREEN_H

///////////////////////////////////////////////////////////////////////////////////////
////// VIEWSCREEN.H
////// Declarations of ViewScreen class. This class is in charge of converting
////// a pixel on the screen (which is dictated by its Mesh to be integrated)
////// into physical initial conditions for the geodesic.
////// All definitions in ViewScreen.cpp
///////////////////////////////////////////////////////////////////////////////////////

#include "Geometry.h" // For basic tensor objects
#include "Metric.h" // For the Metric object
#include "Mesh.h" // For the Mesh object

#include <memory> // std::unique_ptr
#include <utility> // std::move
#include <array> // std::array
#include <string> // strings


// Type of geodesic being integrated. NOTE: only Null supported/implemented at the moment!
enum class GeodesicType
{
	Null = 0,
	Timelike = -1,
	Spacelike = 1,
};

// ViewScreen class: this class is in charge of converting a pixel on the screen (which the Mesh wants to integrate)
// to physical initial conditions for the position and velocity of a geodesic. It owns a Mesh instance, which will tell it
// which pixels to integrate etc.
class ViewScreen
{
public:
	// No default constructor possible
	ViewScreen() = delete;
	// constructor must pass following arguments along:
	// - physical position and looking direction;
	// - screen dimensions (in dimensions of length)
	// - the Mesh used (ViewScreen must become a owner of this object!)
	// - the Metric used (ViewScreen is NOT the owner of the Metric)
	// - the geodesic type to be integrated (null, timelike, spacelike)
	ViewScreen(Point pos, OneIndex dir, ScreenPoint screensize, ScreenPoint screencenter,
		std::unique_ptr<Mesh> theMesh, const Metric* const theMetric, GeodesicType thegeodtype=GeodesicType::Null) 
		: m_Pos{ pos }, m_Direction{ dir }, m_ScreenSize{ screensize }, m_ScreenCenter{ screencenter },
		m_theMesh{ std::move(theMesh) },
		m_theMetric{theMetric},	m_GeodType{ thegeodtype },
		m_rLogScale{ dynamic_cast<const SphericalHorizonMetric*>(theMetric) 
			&& (dynamic_cast<const SphericalHorizonMetric*>(theMetric))->getrLogScale() }
	{
		// At the moment, we don't even use the direction; we are always pointed towards the origin
		if (m_Direction != Point{ 0,-1,0,0 })
		{
			ScreenOutput("ViewScreen is only supported pointing inwards at the moment; Direction = {0, -1, 0, 0} will be used",
				OutputLevel::Level_0_WARNING);
		}
		// At the moment, we are only integrating null geodesics
		if (m_GeodType != GeodesicType::Null)
		{
			ScreenOutput("ViewScreen only supports null geodesics at the moment; geodesics integrated will be null.",
				OutputLevel::Level_0_WARNING);
		}

		// Investigate metric at the camera position
		if (m_theMetric)
		{
			TwoIndex metricpos = m_theMetric->getMetric_uu(m_Pos);
			// If there are g^{r\nu} (with \nu\neq r) cross terms, then our procedure is strictly speaking not correct!
			// We use p_r = 0 implicitly when raising indices to obtain the geodesic's initial velocity
			if (metricpos[1][0] != 0 || metricpos[1][2] != 0 || metricpos[1][3] != 0)
				ScreenOutput("ViewScreen: inverse metric has cross terms of the form g^{r a} (with a<>r)! Initial conditions of geodesic will not be strictly correct!", OutputLevel::Level_0_WARNING);
		}
	}

	// Heart of the ViewScreen: here, the ViewScreen is asked to provide initial conditions
	// for the geodesic nr index of the current iteration; based on the screen index
	// that the Mesh gives, it sets up these physical initial conditions.
	void SetNewInitialConditions(largecounter index, Point& pos, OneIndex& vel, ScreenIndex& scrIndex) const;

	// These member functions essentially pass on information to/from the Mesh
	bool IsFinished() const; // Does the ViewScreen (i.e. the Mesh) want to integrate more geodesics or not?
	largecounter getCurNrGeodesics() const; // Current number of geodesics in this iteration
	void EndCurrentLoop(); // The current iteration of geodesics is finished; prepare the next one
	// NOTE: despite not being const, this function has been designed to be threadsafe!
	void GeodesicFinished(largecounter index, std::vector<real> finalValues); // This geodesic has been integrated, returning its final "values"

	// Description string getter (spaces allowed), also will contain information about the Mesh
	std::string getFullDescriptionStr() const;

private:
	// The position and looking direction of the camera
	const Point m_Pos;
	const OneIndex m_Direction;
	// The screensize (in physical units of length)
	const ScreenPoint m_ScreenSize;
	// The screen center
	const ScreenPoint m_ScreenCenter;

	// Whether the metric uses a logarithmic r coordinate or not
	const bool m_rLogScale;

	// const pointer to const Metric
	const Metric* const m_theMetric;

	// The geodesic type to be integrated
	const GeodesicType m_GeodType{ GeodesicType::Null };

	// The const pointer to the Mesh we are using to determine pixels to be integrated
	const std::unique_ptr<Mesh> m_theMesh;
};



#endif
