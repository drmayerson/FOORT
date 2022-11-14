#ifndef _FOORT_VIEWSCREEN_H
#define _FOORT_VIEWSCREEN_H

#include"Geometry.h"
#include"Metric.h"
#include"Mesh.h"

#include<array>
#include<memory>

enum class GeodesicType
{
	Null = 0,
	Timelike = -1,
	Spacelike = 1,
};

class ViewScreen
{
public:
	ViewScreen( Point pos, OneIndex dir, std::array<real, dimension - 2> screensize,
		std::unique_ptr<Mesh> theMesh, const Metric* theMetric, GeodesicType thegeodtype=GeodesicType::Null) 
		: m_Pos{ pos }, m_Direction{ dir }, m_ScreenSize{ screensize }, m_theMesh{ std::move(theMesh) },
		m_theMetric{theMetric},	m_GeodType{ thegeodtype }
	{
		if (m_Direction != Point{ 0,-1,0,0 })
		{
			ScreenOutput("Warning: ViewScreen is only supported pointing inwards at the moment; Direction = {0, -1, 0, 0} will be used",
				OutputLevel::Level_1_PROC);
			m_Direction = Point{ 0,-1,0,0 };
		}
		if (m_ScreenSize[1] != m_ScreenSize[0])
		{
			ScreenOutput("At the moment only considering square screens. Setting the height equal to width.", OutputLevel::Level_1_PROC);
			m_ScreenSize[1] = m_ScreenSize[0];
		}
		if (m_GeodType != GeodesicType::Null)
		{
			ScreenOutput("Warning: ViewScreen only supports null geodesics at the moment; setting geodesics to null.");
			m_GeodType = GeodesicType::Null;
		}


		TwoIndex metricpos = m_theMetric->getMetric_uu(m_Pos);
		// If there are g^{r\nu} (with \nu\neq r) cross terms, then our procedure is strictly speaking not correct!
		// We use p_r = 0 implicitly when raising indices to obtain the geodesic's initial velocity
		if (metricpos[1][0] != 0 || metricpos[1][2] != 0 || metricpos[1][3] != 0)
			ScreenOutput("Warning: inverse metric has cross terms of the form g^{r a} (with a<>r)! Initial conditions of geodesic will not be strictly speaking correct!", OutputLevel::Level_1_PROC);
	}

	void SetNewInitialConditions(int index, Point& pos, OneIndex& vel, ScreenIndex& scrIndex);

	bool IsFinished();

	int getCurNrGeodesics();

	void EndCurrentLoop();

	void GeodesicFinished(int index, std::vector<real> finalValues);

private:
	Point m_Pos;
	OneIndex m_Direction;
	std::array<real, dimension - 2> m_ScreenSize;

	const Metric* m_theMetric;

	GeodesicType m_GeodType{ GeodesicType::Null };

	std::unique_ptr<Mesh> m_theMesh;
};



#endif
