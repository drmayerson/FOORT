#ifndef _FOORT_MESH_H
#define _FOORT_MESH_H

#include<cassert>

#include"Geometry.h"
#include"Diagnostics.h"
#include<vector>




class Mesh
{

public:
	// Calling CreateDiagnosticVector in this way will create a vector with exactly one element in it,
	// i.e. the Diagnostic we need!
	Mesh(DiagBitflag valdiag) 
		: m_DistanceDiagnostic{ std::move( (CreateDiagnosticVector(valdiag,valdiag))[0] ) }
	{}

	virtual ~Mesh() = default;

	virtual bool IsFinished() const = 0;
	virtual void getNewInitConds(int index, ScreenPoint& newunitpoint, ScreenIndex& newscreenindex) = 0;

	virtual void EndCurrentLoop() = 0;

	virtual int getCurNrGeodesics() const = 0;

	virtual void GeodesicFinished(int index, std::vector<real> finalValues) = 0;

protected:
	std::unique_ptr<Diagnostic> m_DistanceDiagnostic;
};

class SimpleSquareMesh : public Mesh
{
public:
	SimpleSquareMesh(int totalPixels, DiagBitflag valdiag)
		: m_TotalPixels{ static_cast<int>(sqrt(totalPixels)) * static_cast<int>(sqrt(totalPixels)) },
		  m_RowColumnSize{static_cast<int>(sqrt(totalPixels))},
		  Mesh(valdiag)
	{ assert(dimension == 4 && "SimpleSquareMesh only defined in 4D!"); }

	bool IsFinished() const override;
	void getNewInitConds(int index,ScreenPoint &newunitpoint, ScreenIndex &newscreenindex) override;

	void EndCurrentLoop() override;

	int getCurNrGeodesics() const override;

	void GeodesicFinished(int index, std::vector<real> finalValues) override;

protected:
	const int m_TotalPixels;
	const int m_RowColumnSize;
	int m_CurrentPixel{ 0 };
};

class InputCertainPixelsMesh : public Mesh
{
public:
	// Constructor given in cpp file; constructor asks for input of pixels
	InputCertainPixelsMesh(int totalPixels, DiagBitflag valdiag);

	bool IsFinished() const override;
	void getNewInitConds(int index, ScreenPoint& newunitpoint, ScreenIndex& newscreenindex) override;

	void EndCurrentLoop() override;

	int getCurNrGeodesics() const override;

	void GeodesicFinished(int index, std::vector<real> finalValues) override;

protected:
	int m_TotalPixels{ 0 };
	const int m_RowColumnSize;
	std::vector<ScreenIndex> m_PixelsToIntegrate{};
	int m_CurrentPixel{ 0 };
};


class SquareSubdivisionMesh : public Mesh
{
public:
	SquareSubdivisionMesh(int maxPixels, int initialPixels, int maxSubdivide, int iterationPixels, DiagBitflag valdiag)
		: m_InitialPixels{ static_cast<int>(sqrt(initialPixels)) * static_cast<int>(sqrt(initialPixels)) },
		 m_MaxSubdivide{maxSubdivide}, m_RowColumnSize{ (static_cast<int>(sqrt(initialPixels))-1) * ExpInt(2,maxSubdivide-1) + 1 },
		m_PixelsLeft{ maxPixels }, m_IterationPixels{ iterationPixels }, Mesh(valdiag)
	{
		assert(dimension == 4 && "SquareSubdivisionMesh only defined in 4D!");

		if (maxPixels < 0)
			m_InfinitePixels = true;

		ScreenOutput("SquareSubdivisionMesh constructed: maxPixels: " + (m_InfinitePixels ? "infinite" : std::to_string(maxPixels))
			+ "; m_InitialPixels: "
			+ std::to_string(m_InitialPixels) + "; m_RowColumnSize: " + std::to_string(m_RowColumnSize), OutputLevel::Level_4_DEBUG);

		InitializeFirstGrid();
	}

	bool IsFinished() const override;
	void getNewInitConds(int index, ScreenPoint& newunitpoint, ScreenIndex& newscreenindex) override;

	void EndCurrentLoop() override;

	int getCurNrGeodesics() const override;

	void GeodesicFinished(int index, std::vector<real> finalValues) override;

protected:
	struct PixelInfo
	{
		PixelInfo(ScreenIndex ind, int subdiv) : Index{ ind }, SubdivideLevel{ subdiv } {}
		ScreenIndex Index{};
		real Weight{-1};
		int SubdivideLevel{};
		std::vector<real> DiagValue{};
		int LowerNbrIndex{ 0 };
		int RightNbrIndex{ 0 };
	};

	// Initializes the first nxn screen in m_CurrentPixelQueue
	void InitializeFirstGrid();

	// Updates all weights of the pixels in m_AllPixels with weight = 0 and subdiv >0 and subdiv < m_MaxSubdivide
	// Assumes all squares have neighbors assigned correctly
	void UpdateAllWeights();

	// Updates the neighbors of all pixels (that should have neighbors and don't) in m_AllPixels
	void UpdateAllNeighbors();

	// This will take the pixel with index ind from m_AllPixels and subdivide it,
	// adding up to <=5m pixels to the CurrentPixelQueue
	void SubdivideAndQueue(int ind);


	// Helper function to exponentiate an int to an int
	int ExpInt(int base, int exp);

	const int m_MaxSubdivide;
	const int m_InitialPixels;
	const int m_RowColumnSize;
	const int m_IterationPixels;

	bool m_InfinitePixels;
	int m_PixelsLeft;
	std::vector<PixelInfo> m_CurrentPixelQueue{};
	std::vector<bool> m_CurrentPixelQueueDone{};
	std::vector<PixelInfo> m_AllPixels{};
};



#endif