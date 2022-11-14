#include"Mesh.h"
#include"InputOutput.h"

#include<cassert>
#include<limits>


bool SimpleSquareMesh::IsFinished() const
{
	if (m_CurrentPixel == m_TotalPixels-1)
		return true;
	else
		return false;
}

void SimpleSquareMesh::getNewInitConds(int index, ScreenPoint& newunitpoint, ScreenIndex& newscreenindex)
{
	// We should not be getting new initial conditions if all pixels are done already!
	assert(m_CurrentPixel < m_TotalPixels);

	// get (row, column) where each is between 0 and m_RowColumnSize-1 (m_RowColumnSize^2 = m_TotalPixels)
	// Note: m_CurrentPixel runs between 0 and m_TotalPixels-1
	int row{ 0 };
	while ((row+1) * m_RowColumnSize <= m_CurrentPixel)
		++row;

	int column = m_CurrentPixel - row * m_RowColumnSize;

	// Make sure to go to next pixel
	++m_CurrentPixel;

	// Return a 2D ScreenPoint (x,y) with both coordinates between 0 and 1, where 0 and 1 represent the edges of the viewscreen
	//ScreenOutput("SimpleSquareMesh initializer: index " + std::to_string(index) + " initialized to ("
	//	+ std::to_string(row) + ", " + std::to_string(column) + "). Point returned: "
	//	+ toString(ScreenPoint{ row * 1.0 / static_cast<real>(m_RowColumnSize-1),column * 1.0 / static_cast<real>(m_RowColumnSize - 1) }),
	//	OutputLevel::Level_4_DEBUG);
	newscreenindex = ScreenIndex{ row,column };
	newunitpoint=ScreenPoint{ row * 1.0 / static_cast<real>(m_RowColumnSize-1), column * 1.0 / static_cast<real>(m_RowColumnSize - 1) };
}

void SimpleSquareMesh::EndCurrentLoop()
{
	// This Mesh does not need to do anything at the end of the loop. However, all pixels should have been initialized at the end!
	assert(m_CurrentPixel == m_TotalPixels-1 && "Not all pixels have been initialized!");
}

int SimpleSquareMesh::getCurNrGeodesics() const
{
	// This Mesh only has one loop, so the total pixels is the current number of pixels to be made.
	return m_TotalPixels;
}

void SimpleSquareMesh::GeodesicFinished(int index, std::vector<real> finalValues)
{
	// This Mesh doesn't actually have to do anything with the geodesic (values) when it's done!
}

InputCertainPixelsMesh::InputCertainPixelsMesh(int totalPixels, DiagBitflag valdiag) :
m_RowColumnSize{ static_cast<int>(sqrt(totalPixels)) },
Mesh(valdiag)
{
	assert(dimension == 4 && "InputCertainPixelsMesh only defined in 4D!");
	std::string prefix{ "InputCertainPixelsMesh message: " };
	OutputLevel outputlvl{ OutputLevel::Level_0_NONE };

	ScreenOutput(prefix + " Screen is a square with width/height = " + std::to_string(m_RowColumnSize) + ".",
		outputlvl);

	bool finished{ false };
	while (!finished)
	{
		int newx{}, newy{};
		ScreenOutput(prefix + "Please enter coordinates for a pixel (x and y, separated by space); coordinates must lie between 0 and "
			+ std::to_string(m_RowColumnSize - 1) + " (enter negative number to stop): ", outputlvl);
		std::cin >> newx;
		if (newx < 0)
		{
			finished = true;
			break;
		}
		std::cin >> newy;
		if (newy < 0)
		{
			finished = true;
			break;
		}
		if (newx > m_RowColumnSize - 1 || newy > m_RowColumnSize - 1)
		{
			ScreenOutput(prefix + "Invalid coordinates. Please try again.", outputlvl);
		}
		else
		{
			++m_TotalPixels;
			m_PixelsToIntegrate.push_back(ScreenIndex{ newx,newy });
			ScreenOutput(prefix + "Pixel (" + std::to_string(newx) + ", " + std::to_string(newy) + ") added.",
				OutputLevel::Level_0_NONE);
		}
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		std::cin.clear();
	}

	assert(m_TotalPixels > 0 && "No pixels added to integration list!");

}

int InputCertainPixelsMesh::getCurNrGeodesics() const
{
	// This Mesh only has one loop, so the total pixels is the current number of pixels to be made.
	return m_TotalPixels;
}

void InputCertainPixelsMesh::EndCurrentLoop()
{
	// This Mesh does not need to do anything at the end of the loop. However, all pixels should have been initialized at the end!
	assert(m_CurrentPixel == m_TotalPixels - 1 && "Not all pixels have been initialized!");
}

void InputCertainPixelsMesh::GeodesicFinished(int index, std::vector<real> finalValues)
{
	// This Mesh doesn't actually have to do anything with the geodesic (values) when it's done!
}

bool InputCertainPixelsMesh::IsFinished() const
{
	if (m_CurrentPixel == m_TotalPixels - 1)
		return true;
	else
		return false;
}

void InputCertainPixelsMesh::getNewInitConds(int index, ScreenPoint& newunitpoint, ScreenIndex& newscreenindex)
{
	// We should not be getting new initial conditions if all pixels are done already!
	assert(m_CurrentPixel < m_TotalPixels);

	// The next pixel in line
	newscreenindex = m_PixelsToIntegrate[m_CurrentPixel];

	// Make sure to go to next pixel
	++m_CurrentPixel;

	// Return a 2D ScreenPoint (x,y) with both coordinates between 0 and 1, where 0 and 1 represent the edges of the viewscreen
	//ScreenOutput("SimpleSquareMesh initializer: index " + std::to_string(index) + " initialized to ("
	//	+ std::to_string(row) + ", " + std::to_string(column) + "). Point returned: "
	//	+ toString(ScreenPoint{ row * 1.0 / static_cast<real>(m_RowColumnSize-1),column * 1.0 / static_cast<real>(m_RowColumnSize - 1) }),
	//	OutputLevel::Level_4_DEBUG);
	newunitpoint = ScreenPoint{ newscreenindex[0] * 1.0 / static_cast<real>(m_RowColumnSize - 1), 
		newscreenindex[1] * 1.0 / static_cast<real>(m_RowColumnSize - 1)};
}

/// <summary>
/// SquareSubdivisionMesh
/// </summary>


int SquareSubdivisionMesh::ExpInt(int base, int exp)
{
	int ret{ 1 };
	while (exp > 0)
	{
		ret *= base;
		--exp;
	}
	return ret;
}


int SquareSubdivisionMesh::getCurNrGeodesics() const
{
	return m_CurrentPixelQueue.size();
}

void SquareSubdivisionMesh::getNewInitConds(int index, ScreenPoint& newunitpoint, ScreenIndex& newscreenindex)
{
	newscreenindex = m_CurrentPixelQueue[index].Index;
	newunitpoint = ScreenPoint{ newscreenindex[0] * 1.0 / static_cast<real>(m_RowColumnSize - 1),
		newscreenindex[1] * 1.0 / static_cast<real>(m_RowColumnSize - 1) };

	//ScreenOutput("SquareSubdivisionMesh sending pixel " + toString(newscreenindex) + " for integration.", OutputLevel::Level_4_DEBUG);

}


bool SquareSubdivisionMesh::IsFinished() const
{
	return m_CurrentPixelQueue.size() == 0;
}

void SquareSubdivisionMesh::SubdivideAndQueue(int ind)
{
	// Helper function to find a pixel
	auto FindPos = [this](int row, int col)
	{
		return std::find_if(m_AllPixels.begin(), m_AllPixels.end(),
			[row, col](const PixelInfo& pixel)
			{ return pixel.Index[0] == row && pixel.Index[1] == col; });
	};

	int newsubdiv = m_AllPixels[ind].SubdivideLevel + 1;
	int row{}, col{};

	// The pixel itself now has the new subdivision level
	m_AllPixels[ind].SubdivideLevel = newsubdiv;
	m_AllPixels[ind].Weight = -1;
	m_AllPixels[ind].LowerNbrIndex = 0;
	m_AllPixels[ind].RightNbrIndex = 0;

	// new right neighbor (+0,+1) -- will have neighbors
	row = m_AllPixels[ind].Index[0];
	col = m_AllPixels[ind].Index[1] + ExpInt(2, m_MaxSubdivide - newsubdiv);
	auto newpos = FindPos(row, col);
	if (newpos == m_AllPixels.end())
	{
		// New pixel to integrate; weight is automatically 0
		m_CurrentPixelQueue.push_back(PixelInfo(ScreenIndex{ row, col }, newsubdiv));
	}
	else
	{
		// This pixel exists already, but will now have a neighbor
		newpos->SubdivideLevel = newsubdiv;
		newpos->Weight = -1;
		newpos->LowerNbrIndex = 0;
		newpos->RightNbrIndex = 0;
	}

	// new lower neighbor (+1,+0) -- will have neighbors
	row = m_AllPixels[ind].Index[0] + ExpInt(2, m_MaxSubdivide - newsubdiv);
	col = m_AllPixels[ind].Index[1];
	newpos = FindPos(row, col);
	if (newpos == m_AllPixels.end())
	{
		// New pixel to integrate; weight is automatically 0
		m_CurrentPixelQueue.push_back(PixelInfo(ScreenIndex{ row, col }, newsubdiv));
	}
	else
	{
		// This pixel exists already, but will now have a neighbor
		newpos->SubdivideLevel = newsubdiv;
		newpos->Weight = -1;
		newpos->LowerNbrIndex = 0;
		newpos->RightNbrIndex = 0;
	}

	// new middle pixel (+1,+1) -- will have neighbors
	row = m_AllPixels[ind].Index[0] + ExpInt(2, m_MaxSubdivide - newsubdiv);
	col = m_AllPixels[ind].Index[1] + ExpInt(2, m_MaxSubdivide - newsubdiv);
	newpos = FindPos(row, col);
	if (newpos == m_AllPixels.end())
	{
		// New pixel to integrate; weight is automatically -1
		m_CurrentPixelQueue.push_back(PixelInfo(ScreenIndex{ row, col }, newsubdiv));
	}
	else
	{
		// This pixel exists already, but will now have a neighbor
		newpos->SubdivideLevel = newsubdiv;
		newpos->Weight = -1;
		newpos->LowerNbrIndex = 0;
		newpos->RightNbrIndex = 0;
	}

	// new lower-middle pixel (+2,+1) -- will NOT have neighbor
	row = m_AllPixels[ind].Index[0] + 2 * ExpInt(2, m_MaxSubdivide - newsubdiv);
	col = m_AllPixels[ind].Index[1] + ExpInt(2, m_MaxSubdivide - newsubdiv);
	newpos = FindPos(row, col);
	if (newpos == m_AllPixels.end())
	{
		// New pixel to integrate; weight is automatically 0;
		// subdivision = 0 because no neighbor!
		m_CurrentPixelQueue.push_back(PixelInfo(ScreenIndex{ row, col }, 0));
	}
	// no else: no new neighbor for this pixel!

	// new middle-right pixel (+1,+2) -- will NOT have neighbor
	row = m_AllPixels[ind].Index[0] + ExpInt(2, m_MaxSubdivide - newsubdiv);
	col = m_AllPixels[ind].Index[1] + 2 * ExpInt(2, m_MaxSubdivide - newsubdiv);
	newpos = FindPos(row, col);
	if (newpos == m_AllPixels.end())
	{
		// New pixel to integrate; weight is automatically -1
		// subdivision = 0 because no neighbor!
		m_CurrentPixelQueue.push_back(PixelInfo(ScreenIndex{ row, col }, 0));
	}
	// no else: no new neighbor for this pixel!

	// We are done: the original pixel has its weight & subdivision updated,
	// and the 5 "new" pixels have either been added to the queue for integration,
	// or if they already exist, their subdivision/weight have been updated accordingly
}

void SquareSubdivisionMesh::EndCurrentLoop()
{
	// First, we make sure all geodesics have indeed been integrated
	bool alldone{ true };
	for (int i = 0; i < m_CurrentPixelQueueDone.size() && alldone; ++i)
	{
		if (!m_CurrentPixelQueueDone[i])
		{
			alldone = false;
		}
	}
	assert(alldone && "Not all pixels have been integrated!");

	// Next, we move all the pixels in CurrentPixelQueue to AllPixels
	// and update the necessary neighbors of these pixels, then make CurrentPixelQueue empty
	if (m_AllPixels.size() == 0)
	{
		m_AllPixels = m_CurrentPixelQueue;
	}
	else
	{
		m_AllPixels.insert(m_AllPixels.end(), m_CurrentPixelQueue.begin(), m_CurrentPixelQueue.end());
	}
	// pixel queue is now empty
	m_CurrentPixelQueue = std::vector<PixelInfo>{};
	m_CurrentPixelQueueDone = std::vector<bool>{};

	// Now, we want to create a new pixel queue, but only if there are pixels left to integrate
	if (m_PixelsLeft > 0)
	{
		// Update all neighbors and weights
		UpdateAllNeighbors();
		UpdateAllWeights();

		// Select new subdivide vertices (m)
		std::vector<int> DivideVertexIndices{};
		DivideVertexIndices.reserve(m_AllPixels.size());
		// first, select all allowed candidates; allowed candidates must have weight > 0!
		for (int i=0; i<m_AllPixels.size(); ++i)
		{
			if (m_AllPixels[i].SubdivideLevel > 0 && m_AllPixels[i].SubdivideLevel < m_MaxSubdivide && m_AllPixels[i].Weight > 0)
			{
				DivideVertexIndices.push_back(i);
			}
		}
		// Now, order the candidates according to how much we want to subdivide them (front is most important)
		auto Comp = [this](int ind1, int ind2) -> bool // returns true if ind1 is more important than ind2
		{
			if (m_AllPixels[ind1].Weight > m_AllPixels[ind2].Weight)
				return true;
			else if (m_AllPixels[ind1].Weight == m_AllPixels[ind2].Weight 
				&& m_AllPixels[ind1].SubdivideLevel < m_AllPixels[ind2].SubdivideLevel)
				return true;
			else
				return false;
		};
		std::sort(DivideVertexIndices.begin(), DivideVertexIndices.end(), Comp);
		// Now, only keep the first m of these if we have too many
		if(DivideVertexIndices.size() > m_IterationPixels)
			DivideVertexIndices.erase(DivideVertexIndices.begin() + m_IterationPixels, DivideVertexIndices.end());		

		// At most, we will be creating 5x this number of new pixels to integrate
		m_CurrentPixelQueue.reserve(5 * DivideVertexIndices.size());
		// Populate the queue with <=5*m_IterationPixels to integrate
		for (int ind : DivideVertexIndices)
		{
			SubdivideAndQueue(ind);
		}

		// If the queue is too large, truncate it
		// We choose to delete the last elements. These should be the least important in the queue
		if (m_CurrentPixelQueue.size() > m_PixelsLeft)
			m_CurrentPixelQueue.erase(m_CurrentPixelQueue.begin() + m_PixelsLeft, m_CurrentPixelQueue.end());

		// Queue is constructed now, make sure to subtract the pixels from the total we have left
		// and initialize m_CurrentPixelQueueDone
		m_PixelsLeft -= m_CurrentPixelQueue.size();
		m_CurrentPixelQueueDone = std::vector<bool>(m_CurrentPixelQueue.size(), false);
	}

	// Our queue is ready for integration now!
	// if no pixels are left to integrate, the queue will have remained empty (and IsFinished() will return true)
	// (the same is true if we have not actually managed to "create" any new pixels in the subdivision process)
}

void SquareSubdivisionMesh::UpdateAllNeighbors()
{

	for (auto& pixel : m_AllPixels)
	{
		// first, make sure we actually are allowed to have neighbors, and that they need updating
		if (pixel.SubdivideLevel > 0 && pixel.LowerNbrIndex == 0 && pixel.RightNbrIndex == 0)
		{
			int row{}, col{};

			// Find right neighbor
			row = pixel.Index[0];
			col = pixel.Index[1] + ExpInt(2, m_MaxSubdivide - pixel.SubdivideLevel);
			auto rightloc = std::find_if(m_AllPixels.begin(), m_AllPixels.end(),
				[row, col](const PixelInfo& p)
				{ return p.Index[0] == row && p.Index[1] == col; });
			// The neighbor should always exist if everything proceeded correctly!
			assert(rightloc != m_AllPixels.end() && "Something went wrong. Pixel "
				+ toString(pixel.Index) + " does not have a right neighbor!");
			pixel.RightNbrIndex = rightloc - m_AllPixels.begin();

			// Find lower neighbor
			row = pixel.Index[0] + ExpInt(2, m_MaxSubdivide - pixel.SubdivideLevel);
			col = pixel.Index[1];
			auto lowloc = std::find_if(m_AllPixels.begin(), m_AllPixels.end(),
				[row, col](const PixelInfo& p)
				{ return p.Index[0] == row && p.Index[1] == col; });
			// The neighbor should always exist if everything proceeded correctly!
			assert(lowloc != m_AllPixels.end() && "Something went wrong. Pixel "
				+ toString(pixel.Index) + " does not have a lower neighbor!");
			pixel.LowerNbrIndex = lowloc - m_AllPixels.begin();
		}
	}
}

void SquareSubdivisionMesh::UpdateAllWeights()
{
	// Updates all weights of the pixels in m_AllPixels with weight = 0 and subdiv > 0 and subdiv < m_MaxSubdivide
	// Assumes all squares have neighbors assigned correctly

	for (auto& pixel : m_AllPixels)
	{
		if (pixel.Weight < 0 && pixel.SubdivideLevel > 0 && pixel.SubdivideLevel < m_MaxSubdivide)
		{
			// update weight
			std::array<real, 3> distances{};

			// (up-left) - (up-right)
			distances[0] = m_DistanceDiagnostic->FinalDataValDistance( pixel.DiagValue, m_AllPixels[pixel.RightNbrIndex].DiagValue );

			// (up-left) - (down-left)
			distances[1] = m_DistanceDiagnostic->FinalDataValDistance(pixel.DiagValue, m_AllPixels[pixel.LowerNbrIndex].DiagValue);

			// (up-left) - (down-right) (make sure we don't accidentally compare to upper/left edge by using index 0!)
			if (pixel.RightNbrIndex > 0 && m_AllPixels[pixel.RightNbrIndex].LowerNbrIndex > 0)
				distances[2] = m_DistanceDiagnostic->FinalDataValDistance(pixel.DiagValue,
					m_AllPixels[m_AllPixels[pixel.RightNbrIndex].LowerNbrIndex].DiagValue);
			else if (pixel.LowerNbrIndex > 0 && m_AllPixels[pixel.LowerNbrIndex].RightNbrIndex > 0)
				distances[2] = m_DistanceDiagnostic->FinalDataValDistance(pixel.DiagValue,
					m_AllPixels[m_AllPixels[pixel.LowerNbrIndex].RightNbrIndex].DiagValue);
			else
				distances[2] = distances[1];
			
			// Assign as weight the max of all these different distances
			pixel.Weight = *std::max_element(distances.begin(), distances.end());
		}
	}
}

void SquareSubdivisionMesh::GeodesicFinished(int index, std::vector<real> finalValues)
{
	m_CurrentPixelQueue[index].DiagValue = finalValues;
	m_CurrentPixelQueueDone[index] = true;
}


void SquareSubdivisionMesh::InitializeFirstGrid()
{
	// initial square grid of m_InitialPixels
	int initRowColSize = static_cast<int>(sqrt(m_InitialPixels));
	m_CurrentPixelQueue.reserve(m_InitialPixels);

	for (int i = 0; i < m_InitialPixels; ++i)
	{
		int row{ 0 };
		while ((row + 1) * initRowColSize <= i)
			++row;
		int column = i - row * initRowColSize;
		// Initialize vertex to subdivision level 1, except if vertex is at lower or rightmost edges 
		int subdiv = 1;	
		if (row == initRowColSize - 1 || column == initRowColSize - 1)
			subdiv = 0;
		// initialize vertex (weight is automatically -1)
		m_CurrentPixelQueue.push_back(PixelInfo(ExpInt(2, m_MaxSubdivide - 1) * ScreenIndex { row, column }, subdiv));
	}

	// Subtract the number of pixels we are integrating, and initialize m_CurrentPixelQueueDone
	m_PixelsLeft -= m_CurrentPixelQueue.size();
	m_CurrentPixelQueueDone = std::vector<bool>(m_CurrentPixelQueue.size(), false);
}