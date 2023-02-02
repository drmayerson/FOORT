#include"Mesh.h" // We are defining Mesh functions here

#include "Utilities.h" // we use the Timer in SquareSubdivisionMeshV2

#include <algorithm> // needed for std::find_if, std::sort, std::max_element, std::min, etc
#include <limits> // for std::numeric_limits
#include <iostream> // for std::cin


/// <summary>
/// Mesh (abstract base class) functions
/// </summary>

std::string Mesh::getFullDescriptionStr() const
{
	return "Mesh (no override description specified)";
}


/// <summary>
/// SimpleSquareMesh functions
/// </summary>


bool SimpleSquareMesh::IsFinished() const
{
	// We are done if we have sent all pixels to be integrated (there is only one iteration of pixels)
	return m_Finished;
}

void SimpleSquareMesh::getNewInitConds(largecounter index, ScreenPoint& newunitpoint, ScreenIndex& newscreenindex) const
{
	// We should not be getting new initial conditions if all pixels are done already!
	if (index >= m_TotalPixels)
	{
		ScreenOutput("Trying to initialize a pixel after all pixels are done!", OutputLevel::Level_0_WARNING);
	}

	// get (row, column) where each is between 0 and m_RowColumnSize-1 (m_RowColumnSize^2 = m_TotalPixels)
	// Note: m_CurrentPixel runs between 0 and m_TotalPixels-1
	pixelcoord row{ 0 };
	while ((row+1) * m_RowColumnSize <= index)
		++row;

	pixelcoord column =  index - row * m_RowColumnSize ;

	// Return a 2D ScreenPoint (x,y) with both coordinates between 0 and 1, where 0 and 1 represent the edges of the viewscreen
	newscreenindex = ScreenIndex{ row,column };
	newunitpoint=ScreenPoint{ row * 1.0 / static_cast<real>(m_RowColumnSize-1), column * 1.0 / static_cast<real>(m_RowColumnSize - 1) };
}

void SimpleSquareMesh::EndCurrentLoop()
{
	// We are now done integrating
	m_Finished = true;
}

largecounter SimpleSquareMesh::getCurNrGeodesics() const
{
	// This Mesh only has one loop, so the total pixels is the current number of pixels to be integrated.
	return m_TotalPixels;
}

void SimpleSquareMesh::GeodesicFinished([[maybe_unused]] largecounter index, [[maybe_unused]] std::vector<real> finalValues)
{
	// This Mesh doesn't actually have to do anything with the geodesic (values) when it is done!
}

std::string SimpleSquareMesh::getFullDescriptionStr() const
{
	// Description string
	return "Mesh: simple square grid (" + std::to_string(m_RowColumnSize) + "^2 pixels)";
}


/// <summary>
/// InputCertainPixelsMesh functions
/// </summary>


InputCertainPixelsMesh::InputCertainPixelsMesh(largecounter totalPixels, DiagBitflag valdiag) :
m_RowColumnSize{ static_cast<pixelcoord>(sqrt(totalPixels)) },
Mesh(valdiag)
{
	if constexpr (dimension!=4)
		ScreenOutput("InputCertainPixelsMesh only defined in 4D!", OutputLevel::Level_0_WARNING);

	// Constructor will ask for pixels to be inputted by the user through the console
	// We want to identify messages from the Mesh with the prefix,
	// and we set the level to be as high as possible since we want to make sure our messages are outputted!
	std::string prefix{ "InputCertainPixelsMesh message: " };
	OutputLevel outputlvl{ OutputLevel::Level_0_WARNING };

	// First, output the total screen size
	ScreenOutput(prefix + "Screen is a square with width/height = " + std::to_string(m_RowColumnSize) + ".",
		outputlvl);

	// Keep asking for new input as long as the user does not want to stop
	bool finished{ false };
	while (!finished)
	{
		// Get new coordinates from the user
		// If user enters negative number somewhere, then this signifies wanting to stop inputting pixels
		// use long long (to allow negative numbers!) and later convert to pixelcoord
		long long newx{}, newy{};
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
		// Check to make sure coordinates are valid. (Note: already checked that they are >=0 above!)
		if (newx > m_RowColumnSize - 1 || newy > m_RowColumnSize - 1)
		{
			ScreenOutput(prefix + "Invalid coordinates. Please try again.", outputlvl);
		}
		else
		{
			// The inputted pixel coordinates are valid. Add this to the list of pixels to integrate
			++m_TotalPixels;
			m_PixelsToIntegrate.push_back(ScreenIndex{ static_cast<pixelcoord>(newx),static_cast<pixelcoord>(newy) });
			ScreenOutput(prefix + "Pixel (" + std::to_string(newx) + ", " + std::to_string(newy) + ") added.",
				outputlvl);
		}
		// Clear the input stream after this pixel to make sure we don't pick up anything else that may have been on this input line
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		std::cin.clear();
	}

	// Make sure we have at least one pixel in the list!
	if (m_TotalPixels == 0)
		ScreenOutput("No pixels added to integration list!", OutputLevel::Level_0_WARNING);

}

largecounter InputCertainPixelsMesh::getCurNrGeodesics() const
{
	// This Mesh only has one loop, so the total pixels is the current number of pixels to be made.
	return m_TotalPixels;
}

void InputCertainPixelsMesh::EndCurrentLoop()
{
	// This Mesh only has one loop; we are now finished integrating
	m_Finished = true;
}

void InputCertainPixelsMesh::GeodesicFinished([[maybe_unused]] largecounter index, [[maybe_unused]] std::vector<real> finalValues)
{
	// This Mesh doesn't actually have to do anything with the geodesic (values) when it's done!
}

bool InputCertainPixelsMesh::IsFinished() const
{
	// The Mesh is finished after one loop
	return m_Finished;
}

void InputCertainPixelsMesh::getNewInitConds(largecounter index, ScreenPoint& newunitpoint, ScreenIndex& newscreenindex) const
{
	// We should not be getting new initial conditions if all pixels are done already!
	if (index >= m_TotalPixels)
		ScreenOutput("Trying to initialize pixel but all pixels are done already!", OutputLevel::Level_0_WARNING);

	// The next pixel in line
	newscreenindex = m_PixelsToIntegrate[index];

	// Return a 2D ScreenPoint (x,y) with both coordinates between 0 and 1, where 0 and 1 represent the edges of the viewscreen
	newunitpoint = ScreenPoint{ newscreenindex[0] * 1.0 / static_cast<real>(m_RowColumnSize - 1), 
		newscreenindex[1] * 1.0 / static_cast<real>(m_RowColumnSize - 1)};
}

std::string InputCertainPixelsMesh::getFullDescriptionStr() const
{
	// Descriptive string
	return "Mesh: User-input pixels";
}


/// <summary>
/// SquareSubdivisionMesh functions
/// </summary>


largecounter SquareSubdivisionMesh::getCurNrGeodesics() const
{
	// The number of geodesics in the current integration iteration
	return static_cast<largecounter>(m_CurrentPixelQueue.size());
}

void SquareSubdivisionMesh::getNewInitConds(largecounter index, ScreenPoint& newunitpoint, ScreenIndex& newscreenindex) const
{
	// Returning the geodesic with the appropriate index in the current queue
	newscreenindex = m_CurrentPixelQueue[index].Index;
	// Return a 2D ScreenPoint (x,y) with both coordinates between 0 and 1, where 0 and 1 represent the edges of the viewscreen
	newunitpoint = ScreenPoint{ newscreenindex[0] * 1.0 / static_cast<real>(m_RowColumnSize - 1),
		newscreenindex[1] * 1.0 / static_cast<real>(m_RowColumnSize - 1) };
}

void SquareSubdivisionMesh::GeodesicFinished(largecounter index, std::vector<real> finalValues)
{
	// NOTE: this function must be thread-safe!
	// This means all the changes it makes are to values in a vector, never re-shaping the vector!
	// "Concurrently accessing or modifying different elements is safe." (from cplusplus.com std::vector::operator[])
	
	// Set this pixel's values to the returned values
	m_CurrentPixelQueue[index].DiagValue = finalValues;
	// This pixels is now done
	m_CurrentPixelQueueDone[index] = true;
}

// Note: definition of SquareSubdivisionMesh::EndCurrentLoop() is below

bool SquareSubdivisionMesh::IsFinished() const
{
	// We are finished if we did not manage to populate the current pixel queue with any new pixels to integrate
	return m_CurrentPixelQueue.size() == 0;
}

std::string SquareSubdivisionMesh::getFullDescriptionStr() const
{
	// Descriptive string
	return "Mesh: square subdivision (initial pixels: "
		+ std::to_string(static_cast<pixelcoord>(sqrt(m_InitialPixels))) + "^2; max subdivision: "
		+ std::to_string(m_MaxSubdivide) + "; pixels subdivided per iteration: " + std::to_string(m_IterationPixels)
		+ "; max total pixels: " + (m_InfinitePixels ? "infinite" : std::to_string(m_MaxPixels))
		+ "; if pixel is initially subdivided, will continue to max: " + std::to_string(m_InitialSubDividideToFinal)
		+ ")";
}

// Helper (private) member function
pixelcoord SquareSubdivisionMesh::ExpInt(int base, int exp)
{
	// Helper function to exponentiate ints; note: the result may be larger than fits in an int, but
	// this is only called with int arguments
	pixelcoord ret{ 1 };
	while (exp > 0)
	{
		ret *= base;
		--exp;
	}
	return ret;
}

//////////////////////////////////////////////////////////////
//// Important SquareSubdivisionMesh functions start here ////


// Helper function: sets up the initial grid to integrate
void SquareSubdivisionMesh::InitializeFirstGrid()
{
	// initial square grid of m_InitialPixels
	pixelcoord initRowColSize = static_cast<pixelcoord>(sqrt(m_InitialPixels));
	m_CurrentPixelQueue.reserve(m_InitialPixels);

	for (largecounter i = 0; i < m_InitialPixels; ++i)
	{
		// Find the pixel's row and column
		pixelcoord row{ 0 };
		while ((row + 1) * initRowColSize <= i)
			++row;
		pixelcoord column = i - row * initRowColSize;

		// Initialize vertex to subdivision level 1, except if vertex is at lower or rightmost edges 
		int subdiv = 1;
		if (row == initRowColSize - 1 || column == initRowColSize - 1)
			subdiv = 0;

		// initialize vertex (weight is automatically initialized to -1)
		// Note that the actual ScreenIndex that the pixel gets put at depends on the max
		// level of subdivision: we are keeping room for all of the potential future pixels 
		// that can come in between the initial grid!
		m_CurrentPixelQueue.push_back(PixelInfo(ExpInt(2, m_MaxSubdivide - 1) * ScreenIndex { row, column }, subdiv));
	}

	// Subtract the number of pixels we are integrating from the pixels we are allowed to integrate
	if (!m_InfinitePixels)
		m_PixelsLeft -= static_cast<largecounter>(m_CurrentPixelQueue.size());

	// All pixels have not been integrated yet
	m_CurrentPixelQueueDone = std::vector<bool>(m_CurrentPixelQueue.size(), false);
}

// Helper function: updates all pixels' neighbors (if pixel can have neighbors and needs updating)
void SquareSubdivisionMesh::UpdateAllNeighbors()
{
	ScreenOutput("Updating all pixel neighbor information...", OutputLevel::Level_3_ALLDETAIL);

	// Loop through all pixels
	for (auto& pixel : m_AllPixels)
	{
		// first, make sure we actually are allowed to have neighbors, and that they need updating
		if (pixel.SubdivideLevel > 0 && pixel.LowerNbrIndex == 0 && pixel.RightNbrIndex == 0)
		{
			pixelcoord row{}, col{};

			// Find right neighbor
			row = pixel.Index[0];
			col = pixel.Index[1] + ExpInt(2, m_MaxSubdivide - pixel.SubdivideLevel);
			auto rightloc = std::find_if(m_AllPixels.begin(), m_AllPixels.end(),
				[row, col](const PixelInfo& p)
				{ return p.Index[0] == row && p.Index[1] == col; });
			// The neighbor should always exist if everything proceeded correctly!
			if (rightloc == m_AllPixels.end())
				ScreenOutput("Something went wrong. Pixel "
					+ toString(pixel.Index) + " does not have a right neighbor!", OutputLevel::Level_0_WARNING);
			pixel.RightNbrIndex = static_cast<largecounter>( rightloc - m_AllPixels.begin() );

			// Find lower neighbor
			row = pixel.Index[0] + ExpInt(2, m_MaxSubdivide - pixel.SubdivideLevel);
			col = pixel.Index[1];
			auto lowloc = std::find_if(m_AllPixels.begin(), m_AllPixels.end(),
				[row, col](const PixelInfo& p)
				{ return p.Index[0] == row && p.Index[1] == col; });
			// The neighbor should always exist if everything proceeded correctly!
			if (lowloc == m_AllPixels.end())
				ScreenOutput("Something went wrong. Pixel "
					+ toString(pixel.Index) + " does not have a lower neighbor!", OutputLevel::Level_0_WARNING);
			pixel.LowerNbrIndex = static_cast<largecounter>( lowloc - m_AllPixels.begin() );
		}
	}

	ScreenOutput("Done updating pixel neighbor information.", OutputLevel::Level_3_ALLDETAIL);
}

// Helper function: updates all weights of pixels that have weight < 0 (so need updating) and have a
// subdivision level that allows further subdivision.
// Note: assumes all squares have neighbors assigned correctly!
void SquareSubdivisionMesh::UpdateAllWeights()
{
	// Updates all weights of the pixels in m_AllPixels with weight < 0 and subdiv > 0 and subdiv < m_MaxSubdivide
	ScreenOutput("Updating all pixel weights...", OutputLevel::Level_3_ALLDETAIL);

	for (auto& pixel : m_AllPixels) // loop through all pixels
	{
		// Check if pixel weight signifies it needs updating,
		// AND pixel subdivision level > 0 (so that it is not on an edge)
		// AND pixel is not already maximally subdivided
		if (pixel.Weight < 0 && pixel.SubdivideLevel > 0 && pixel.SubdivideLevel < m_MaxSubdivide)
		{
			std::array<real, 3> distances{};

			// distance: (up-left) - (up-right)
			distances[0] = m_DistanceDiagnostic->FinalDataValDistance(pixel.DiagValue, m_AllPixels[pixel.RightNbrIndex].DiagValue);

			// distance: (up-left) - (down-left)
			distances[1] = m_DistanceDiagnostic->FinalDataValDistance(pixel.DiagValue, m_AllPixels[pixel.LowerNbrIndex].DiagValue);

			// distance: (up-left) - (down-right) (make sure we don't accidentally compare to upper/left edge by using index 0!)
			if (pixel.RightNbrIndex > 0 && m_AllPixels[pixel.RightNbrIndex].LowerNbrIndex > 0)
			{
				// The pixel's right neighbor is not on the rightmost border
				distances[2] = m_DistanceDiagnostic->FinalDataValDistance(pixel.DiagValue,
					m_AllPixels[m_AllPixels[pixel.RightNbrIndex].LowerNbrIndex].DiagValue);
			}
			else if (pixel.LowerNbrIndex > 0 && m_AllPixels[pixel.LowerNbrIndex].RightNbrIndex > 0)
			{
				// The pixel's right neighbor is on the rightmost border,
				// but its lower neighbor is not on the lowermost border
				distances[2] = m_DistanceDiagnostic->FinalDataValDistance(pixel.DiagValue,
					m_AllPixels[m_AllPixels[pixel.LowerNbrIndex].RightNbrIndex].DiagValue);
			}
			else
			{
				// this should only happen for exactly one pixel (the one in the lower-right corner, one pixel
				// away from the border on the right and the border on the bottom)
				distances[2] = distances[1];
			}

			// Assign as weight the max of these three different distances
			pixel.Weight = *std::max_element(distances.begin(), distances.end());
		}
	}
	ScreenOutput("Done updating pixel weights.", OutputLevel::Level_3_ALLDETAIL);
}

// Helper function: subdivides the square with pixel m_AllPixels[ind] in the upper-left corner,
// and add (up to) 5 new pixels in the integration queue accordingly
void SquareSubdivisionMesh::SubdivideAndQueue(largecounter ind)
{
	// Helper function to find a pixel in m_AllPixels
	auto FindPosBefore = [this](pixelcoord row, pixelcoord col)
	{
		return std::find_if(m_AllPixels.begin(), m_AllPixels.end(),
			[row, col](const PixelInfo& pixel)
			{ return pixel.Index[0] == row && pixel.Index[1] == col; });
	};
	// Helper function to find a pixel in m_CurrentPixelQueue
	auto FindPosCurrent = [this](pixelcoord row, pixelcoord col)
	{
		return std::find_if(m_CurrentPixelQueue.begin(), m_CurrentPixelQueue.end(),
			[row, col](const PixelInfo& pixel)
			{ return pixel.Index[0] == row && pixel.Index[1] == col; });
	};

	// We are subdividing this pixel, so we increase the subdivision level
	int newsubdiv = m_AllPixels[ind].SubdivideLevel + 1;

	// The pixel itself now has the new subdivision level
	m_AllPixels[ind].SubdivideLevel = newsubdiv;
	// We set its neighbors to 0 to indicate it needs to update its neighbors (done in UpdateAllNeighbors())
	m_AllPixels[ind].LowerNbrIndex = 0;
	m_AllPixels[ind].RightNbrIndex = 0;
	// We set its weight < 0 to indicate it needs to update its weight (done in UpdateAllNeighbors())
	m_AllPixels[ind].Weight = -1;

	pixelcoord row{}, col{};

	// new right neighbor (+0,+1) -- will have neighbors
	row = m_AllPixels[ind].Index[0];
	col = m_AllPixels[ind].Index[1] + ExpInt(2, m_MaxSubdivide - newsubdiv);
	auto newpos = FindPosBefore(row, col);
	if (newpos == m_AllPixels.end())
	{
		// New pixel to integrate; weight is automatically 0
		// Only add to queue if not already in queue! If already in queue, check to make sure subdivision is set correctly
		auto curpos = FindPosCurrent(row, col);
		if (curpos == m_CurrentPixelQueue.end())
			m_CurrentPixelQueue.push_back(PixelInfo(ScreenIndex{ row, col }, newsubdiv));
		else
		{
			curpos->SubdivideLevel = std::max(curpos->SubdivideLevel, newsubdiv);
		}
	}
	else
	{
		// This pixel exists already, but will now have neighbors, so we update the subdivision level, weight, and neighbors
		newpos->SubdivideLevel = newsubdiv;
		newpos->Weight = -1;
		newpos->LowerNbrIndex = 0;
		newpos->RightNbrIndex = 0;
	}

	// new lower neighbor (+1,+0) -- will have neighbors
	row = m_AllPixels[ind].Index[0] + ExpInt(2, m_MaxSubdivide - newsubdiv);
	col = m_AllPixels[ind].Index[1];
	newpos = FindPosBefore(row, col);
	if (newpos == m_AllPixels.end())
	{
		// New pixel to integrate; weight is automatically 0
		// Only add to queue if not already in queue! If already in queue, check to make sure subdivision is set correctly
		auto curpos = FindPosCurrent(row, col);
		if (curpos == m_CurrentPixelQueue.end())
			m_CurrentPixelQueue.push_back(PixelInfo(ScreenIndex{ row, col }, newsubdiv));
		else
		{
			curpos->SubdivideLevel = std::max(curpos->SubdivideLevel, newsubdiv);
		}
	}
	else
	{
		// This pixel exists already, but will now have neighbors, so we update the subdivision level, weight, and neighbors
		newpos->SubdivideLevel = newsubdiv;
		newpos->Weight = -1;
		newpos->LowerNbrIndex = 0;
		newpos->RightNbrIndex = 0;
	}

	// new middle pixel (+1,+1) -- will have neighbors
	row = m_AllPixels[ind].Index[0] + ExpInt(2, m_MaxSubdivide - newsubdiv);
	col = m_AllPixels[ind].Index[1] + ExpInt(2, m_MaxSubdivide - newsubdiv);
	newpos = FindPosBefore(row, col);
	if (newpos == m_AllPixels.end())
	{
		// New pixel to integrate; weight is automatically -1
		// Only add to queue if not already in queue! If already in queue, check to make sure subdivision is set correctly
		auto curpos = FindPosCurrent(row, col);
		if (curpos == m_CurrentPixelQueue.end())
			m_CurrentPixelQueue.push_back(PixelInfo(ScreenIndex{ row, col }, newsubdiv));
		else
		{
			curpos->SubdivideLevel = std::max(curpos->SubdivideLevel, newsubdiv);
		}
	}
	else
	{
		// This pixel exists already, but will now have neighbors, so we update the subdivision level, weight, and neighbors
		newpos->SubdivideLevel = newsubdiv;
		newpos->Weight = -1;
		newpos->LowerNbrIndex = 0;
		newpos->RightNbrIndex = 0;
	}

	// new lower-middle pixel (+2,+1) -- will NOT have neighbors!
	row = m_AllPixels[ind].Index[0] + 2 * ExpInt(2, m_MaxSubdivide - newsubdiv);
	col = m_AllPixels[ind].Index[1] + ExpInt(2, m_MaxSubdivide - newsubdiv);
	newpos = FindPosBefore(row, col);
	if (newpos == m_AllPixels.end())
	{
		// New pixel to integrate; weight is automatically 0;
		// subdivision = 0 because no neighbors!
		// Only add to queue if not already in queue!
		if (FindPosCurrent(row, col) == m_CurrentPixelQueue.end())
			m_CurrentPixelQueue.push_back(PixelInfo(ScreenIndex{ row, col }, 0));
	}
	// no else: no new neighbors for this pixel!

	// new middle-right pixel (+1,+2) -- will NOT have neighbors
	row = m_AllPixels[ind].Index[0] + ExpInt(2, m_MaxSubdivide - newsubdiv);
	col = m_AllPixels[ind].Index[1] + 2 * ExpInt(2, m_MaxSubdivide - newsubdiv);
	newpos = FindPosBefore(row, col);
	if (newpos == m_AllPixels.end())
	{
		// New pixel to integrate; weight is automatically -1
		// subdivision = 0 because no neighbor!
		// Only add to queue if not already in queue!
		if (FindPosCurrent(row, col) == m_CurrentPixelQueue.end())
			m_CurrentPixelQueue.push_back(PixelInfo(ScreenIndex{ row, col }, 0));
	}
	// no else: no new neighbor for this pixel!


	// We are done: the original pixel has its weight & subdivision updated,
	// and the 5 "new" pixels have either been added to the queue for integration,
	// or if they already exist, their subdivision/weight have been updated accordingly
}

// This function is called at the end of each integration iteration loop.
// We must wrap up the current iteration and initialize the next one.
void SquareSubdivisionMesh::EndCurrentLoop()
{
	///////////////////////////////////////////
	//// Wrap up of current iteration loop ////
	
	// First, we make sure all geodesics have indeed been integrated
	bool alldone{ true };
	for (largecounter i = 0; i < m_CurrentPixelQueueDone.size() && alldone; ++i)
	{
		if (!m_CurrentPixelQueueDone[i])
		{
			alldone = false;
		}
	}
	if (!alldone)
		ScreenOutput("Not all pixels have been integrated!", OutputLevel::Level_0_WARNING);


	// All pixels in CurrentPixelQueue have been integrated, so we move them to AllPixels
	// and update the necessary neighbors of these pixels, then make CurrentPixelQueue empty
	if (m_AllPixels.size() == 0)
	{
		// This is the first iteration
		m_AllPixels = m_CurrentPixelQueue;
	}
	else
	{
		m_AllPixels.insert(m_AllPixels.end(), m_CurrentPixelQueue.begin(), m_CurrentPixelQueue.end());
	}
	// pixel queue is now empty
	m_CurrentPixelQueue = std::vector<PixelInfo>{};
	m_CurrentPixelQueueDone = std::vector<bool>{};

	ScreenOutput("Total integrated geodesic so far: " + std::to_string(m_AllPixels.size()) + ".", OutputLevel::Level_2_SUBPROC);

	
	//////////////////////////////////////////
	//// Initializing next iteration loop ////

	ScreenOutput("Calculating pixels to subdivide next...", OutputLevel::Level_2_SUBPROC);

	// Now, we want to create a new pixel queue, but only if there are pixels left to integrate
	if (m_InfinitePixels || m_PixelsLeft > 0)
	{
		// Update all neighbors and weights of existing pixels
		UpdateAllNeighbors();
		UpdateAllWeights();

		ScreenOutput("Identifying all possible candidate pixels for subdivision...", OutputLevel::Level_3_ALLDETAIL);

		// Select new subdivide vertices (m)
		std::vector<largecounter> DivideVertexIndices{};
		DivideVertexIndices.reserve(m_AllPixels.size());
		// first, select all allowed candidates; allowed candidates must have weight > 0!
		for (largecounter i = 0; i<m_AllPixels.size(); ++i)
		{
			// The pixel needs to have a subdivision level > 0 (so it is not on an edge) and < max (so that we are allowed to subdivide
			// it further) in order for it to be a candidate. Then, there are two possible criteria:
			// 1) when m_InitialSubDividideToFinal = false, then we only subdivide pixels that have a non-zero weight
			// 2) when m_InitialSubDividideToFinal = true, then we are ALSO allowed to subdivide pixels with a zero weight,
			// IF they have already been subdivided at least once before.
			if (m_AllPixels[i].SubdivideLevel > 0 && m_AllPixels[i].SubdivideLevel < m_MaxSubdivide)
			{ // allowed to subdivide
				if ( m_AllPixels[i].Weight > 0
					|| (m_InitialSubDividideToFinal && m_AllPixels[i].SubdivideLevel > 1) )
				{
					// This pixel is a viable candidate to subdivide
					DivideVertexIndices.push_back(i);
				}
			}
		}

		// All possible candidate pixels to subdivide have been selected. We only need to select the best candidates now.
		ScreenOutput("Selecting pixels for subdivision...", OutputLevel::Level_3_ALLDETAIL);

		// Order the candidates according to how much we want to subdivide them (front is most important)
		auto Comp = [this](largecounter ind1, largecounter ind2) -> bool // returns true if ind1 is more important than ind2
		{
			if (m_AllPixels[ind1].Weight > m_AllPixels[ind2].Weight)
				return true;
			else if (m_AllPixels[ind1].Weight == m_AllPixels[ind2].Weight 
				&& m_AllPixels[ind1].SubdivideLevel < m_AllPixels[ind2].SubdivideLevel)
				return true; // In the case of equal weight, give precedence to less-subdivided pixels
			else
				return false;
		};
		std::sort(DivideVertexIndices.begin(), DivideVertexIndices.end(), Comp);

		// Now, only keep the first m of these if we have too many
		if(DivideVertexIndices.size() > m_IterationPixels)
			DivideVertexIndices.erase(DivideVertexIndices.begin() + m_IterationPixels, DivideVertexIndices.end());		

		// We have now selected the pixels we want to subdivide; now we can actually subdivide them
		ScreenOutput("Setting up subdivided pixels for integration...", OutputLevel::Level_3_ALLDETAIL);
		// At most, we will be creating 5x this number of new pixels to integrate
		m_CurrentPixelQueue.reserve(5 * DivideVertexIndices.size());
		// Populate the queue with <=5*m_IterationPixels to integrate
		for (largecounter ind : DivideVertexIndices)
		{
			SubdivideAndQueue(ind);
		}

		// If the total queue we have created is too large, truncate it
		// We choose to delete the last elements. These should be the least important in the queue due to the ordering process above.
		if (!m_InfinitePixels && m_CurrentPixelQueue.size() > m_PixelsLeft)
			m_CurrentPixelQueue.erase(m_CurrentPixelQueue.begin() + m_PixelsLeft, m_CurrentPixelQueue.end());

		// Queue is constructed now, make sure to subtract the pixels in the queue from the total we have left
		if (!m_InfinitePixels)
		{

			m_PixelsLeft -= static_cast<largecounter>(m_CurrentPixelQueue.size());
			ScreenOutput("Mesh says: Still max. " + std::to_string(m_PixelsLeft) + 
				" left to integrate after the currently queued " + std::to_string(m_CurrentPixelQueue.size()) + ".",
				OutputLevel::Level_2_SUBPROC);
		}
		// Initialize m_CurrentPixelQueueDone
		m_CurrentPixelQueueDone = std::vector<bool>(m_CurrentPixelQueue.size(), false);
	}
	
	ScreenOutput("Done calculating next iteration of pixels.", OutputLevel::Level_2_SUBPROC);
	// Our queue is ready for integration now!
	// if no pixels are left to integrate, the queue will have remained empty (and IsFinished() will now return true)
	// (the same is true if we have not actually managed to "create" any new pixels in the subdivision process)
}


/// <summary>
/// SquareSubdivisionMeshV2 functions
/// </summary>


largecounter SquareSubdivisionMeshV2::getCurNrGeodesics() const
{
	// The number of geodesics in the current integration iteration
	return static_cast<largecounter>(m_CurrentPixelQueue.size());
}

void SquareSubdivisionMeshV2::getNewInitConds(largecounter index, ScreenPoint& newunitpoint, ScreenIndex& newscreenindex) const
{
	// Returning the geodesic with the appropriate index in the current queue
	newscreenindex = m_CurrentPixelQueue[index]->Index;
	// Return a 2D ScreenPoint (x,y) with both coordinates between 0 and 1, where 0 and 1 represent the edges of the viewscreen
	newunitpoint = ScreenPoint{ newscreenindex[0] * 1.0 / static_cast<real>(m_RowColumnSize - 1),
		newscreenindex[1] * 1.0 / static_cast<real>(m_RowColumnSize - 1) };
}

void SquareSubdivisionMeshV2::GeodesicFinished(largecounter index, std::vector<real> finalValues)
{
	// NOTE: this function must be thread-safe!
	// This means all the changes it makes are to values in a vector, never re-shaping the vector!
	// "Concurrently accessing or modifying different elements is safe." (from cplusplus.com std::vector::operator[])
	
	// Set this pixel's values to the returned values
	m_CurrentPixelQueue[index]->DiagValue = std::move(finalValues);
	// This pixels is now done
	m_CurrentPixelQueueDone[index] = true;
}



// Note: definition of SquareSubdivisionMeshV2::EndCurrentLoop() is below

bool SquareSubdivisionMeshV2::IsFinished() const
{
	// We are finished if we did not manage to populate the current pixel queue with any new pixels to integrate
	return m_CurrentPixelQueue.size() == 0;
}

std::string SquareSubdivisionMeshV2::getFullDescriptionStr() const
{
	// Descriptive string
	return "Mesh: square subdivision v2 (initial pixels: "
		+ std::to_string(static_cast<pixelcoord>(sqrt(m_InitialPixels))) + "^2; max subdivision: "
		+ std::to_string(m_MaxSubdivide) + "; pixels subdivided per iteration: " + std::to_string(m_IterationPixels)
		+ "; max total pixels: " + (m_InfinitePixels ? "infinite" : std::to_string(m_MaxPixels))
		+ "; if pixel is initially subdivided, will continue to max: " + std::to_string(m_InitialSubDividideToFinal)
		+ "; row/column size: " + std::to_string(m_RowColumnSize)
		+ ")";
}

// Helper (private) member function
pixelcoord SquareSubdivisionMeshV2::ExpInt(int base, int exp) const
{
	// Helper function to exponentiate ints; note: the result may be larger than fits in an int, but
	// this is only called with int arguments
	pixelcoord ret{ 1 };
	while (exp > 0)
	{
		ret *= base;
		--exp;
	}
	return ret;
}

//////////////////////////////////////////////////////////////
//// Important SquareSubdivisionMeshV2 functions start here ////


// Helper function: sets up the initial grid to integrate
void SquareSubdivisionMeshV2::InitializeFirstGrid()
{
	// initial square grid of m_InitialPixels
	pixelcoord initRowColSize = static_cast<pixelcoord>(sqrt(m_InitialPixels));
	m_CurrentPixelQueue.reserve(m_InitialPixels);

	// Initialize grid of empty pixels, to be filled
	std::vector<std::vector<std::unique_ptr<PixelInfo>>> m_InitialGrid(initRowColSize);
	for (largecounter i = 0; i < initRowColSize; ++i)
		m_InitialGrid[i] = std::vector<std::unique_ptr<PixelInfo>>(initRowColSize);

	// Fill grid with pixels
	for (pixelcoord row = 0; row < initRowColSize; ++row)
	{
		for (pixelcoord column = 0; column < initRowColSize; ++column)
		{
			// Initialize vertex to subdivision level 1, except if vertex is at lower or rightmost edges,
			// then this pixel can never be subdivided
			int subdiv = 1;
			if (row == initRowColSize - 1 || column == initRowColSize - 1)
				subdiv = m_MaxSubdivide;

			// Create pixel (weight is automatically initialized to -1)
			// Note that the actual ScreenIndex that the pixel gets put at depends on the max
			// level of subdivision: we are keeping room for all of the potential future pixels 
			// that can come in between the initial grid!
			m_InitialGrid[row][column] = std::make_unique<PixelInfo>(ExpInt(2, m_MaxSubdivide - 1) * ScreenIndex { row, column }, subdiv);
		}
	}

	// Now, we need to update everybody's neighbors accordingly
	// Note that right/down/SEdiag neighbors are ONLY defined if the pixel is a pixel that can be subdivided;
	// and left/up neighbors are defined to be correct reciprocally with right/down relations
	// The limits of the for loop are such that they only update the neighbors for all the pixels that have them
	for (pixelcoord row = 0; row < initRowColSize - 1; ++row)
	{
		for (pixelcoord column = 0; column < initRowColSize - 1; ++column)
		{
			// Pixel has right/down neighbors, set them and the reciprocals

			m_InitialGrid[row][column]->RightNbr = m_InitialGrid[row][column + 1].get();
			m_InitialGrid[row][column + 1]->LeftNbr = m_InitialGrid[row][column].get();

			m_InitialGrid[row][column]->DownNbr = m_InitialGrid[row + 1][column].get();
			m_InitialGrid[row + 1][column]->UpNbr = m_InitialGrid[row][column].get();

			m_InitialGrid[row][column]->SEdiagNbr = m_InitialGrid[row + 1][column + 1].get();
		}
	}

	// Finally, we move the ownership of all pixels to the master pixel collection
	// and we add all pixels to the integration list and weight-updating list (if applicable)
	for (pixelcoord row = 0; row < initRowColSize; ++row)
	{
		for (pixelcoord column = 0; column < initRowColSize; ++column)
		{
			// This pixel must be integrated
			m_CurrentPixelQueue.push_back(m_InitialGrid[row][column].get());

			// If this pixel can be subdivided, it needs weight updating after integration
			if (m_InitialGrid[row][column]->SubdivideLevel < m_MaxSubdivide)
				m_CurrentPixelUpdating.push_back(m_InitialGrid[row][column].get());

			// Transfer ownership to master pixel collection
			m_AllPixels.push_front(std::move(m_InitialGrid[row][column]));
		}
	}

	// Subtract the number of pixels we are integrating from the pixels we are allowed to integrate
	if (!m_InfinitePixels)
		m_PixelsLeft -= static_cast<largecounter>(m_CurrentPixelQueue.size());

	// All pixels have not been integrated yet
	m_CurrentPixelQueueDone = std::vector<bool>(m_CurrentPixelQueue.size(), false);
}


// Helper function: updates all weights of the pixels in m_CurrentPixelUpdating;
// Note: these are assumed to have subdiv > 0 and subdiv < m_MaxSubdivide, and all their neigbors assigned correctly,
// and are further assumed to have their (and their neighbor's) values assigned correctly
// All pixels with weight > 0 will be added to m_ActivePixels
void SquareSubdivisionMeshV2::UpdateAllWeights()
{
	// Updates all weights of the pixels in m_CurrentPixelUpdating
	ScreenOutput("Updating pixel weights for " + std::to_string(m_CurrentPixelUpdating.size()) + " pixels...",
		OutputLevel::Level_3_ALLDETAIL);

	for (PixelInfo* pixel : m_CurrentPixelUpdating) // loop through all pixels that need weight updating
	{
		std::array<real, 3> distances{};

		// distance: (up-left) - (up-right)
		distances[0] = m_DistanceDiagnostic->FinalDataValDistance(pixel->DiagValue, pixel->RightNbr->DiagValue);

		// distance: (up-left) - (down-left)
		distances[1] = m_DistanceDiagnostic->FinalDataValDistance(pixel->DiagValue, pixel->DownNbr->DiagValue);

		// distance: (up-left) - (down-right)
			// The pixel's right neighbor is not on the rightmost border
		distances[2] = m_DistanceDiagnostic->FinalDataValDistance(pixel->DiagValue, pixel->SEdiagNbr->DiagValue);

		// Assign as weight the max of these three different distances
		pixel->Weight = *std::max_element(distances.begin(), distances.end());
		if (pixel->Weight > 0.0)
		{
			m_ActivePixels.push_back(pixel);
		}
	}

	// Clear m_CurrentPixelUpdating
	m_CurrentPixelUpdating.clear();

	ScreenOutput("Done updating pixel weights.", OutputLevel::Level_3_ALLDETAIL);
}


SquareSubdivisionMeshV2::PixelInfo* SquareSubdivisionMeshV2::GetUp(PixelInfo* p, int subdiv) const
{
	// p does not exist, it does not have the necessary neighbor,
	// or the neighbor lives at a subdivision level that is too low
	// Note that the up-neighbors are defined to be reciprocal with the down-neighbor,
	// therefore we must check the NEIGHBOR's subdivide level (and not p's)!
	if (!p || !p->UpNbr || p->UpNbr->SubdivideLevel < subdiv)
		return nullptr;
	else
	{
		if (p->UpNbr->SubdivideLevel == subdiv)
		{
			// The sought-after neighbor is precisely one step away
			return p->UpNbr;
		}
		else
		{
			// Here, necessarily p->UpNbr->SubdivideLevel > subdiv
			// This means p is subdivided to a higher level than we want
			// In this case, we must travel over this finer grid to find the pixel we are looking for
			return GetUp(GetUp(p, subdiv + 1), subdiv + 1);
		}
	}	
}

SquareSubdivisionMeshV2::PixelInfo* SquareSubdivisionMeshV2::GetDown(PixelInfo* p, int subdiv) const
{
	// p does not exist, it does not have the necessary neighbor,
	// or the neighbor lives at a subdivision level that is too low
	if (!p || !p->DownNbr || p->SubdivideLevel < subdiv)
		return nullptr;
	else
	{
		if (p->SubdivideLevel == subdiv)
		{
			// The sought-after neighbor is precisely one step away
			return p->DownNbr;
		}
		else
		{
			// Here, necessarily p->SubdivideLevel > subdiv
			// This means p is subdivided to a higher level than we want
			// In this case, we must travel over this finer grid to find the pixel we are looking for
			return GetDown(GetDown(p, subdiv + 1), subdiv + 1);
		}
	}
}

SquareSubdivisionMeshV2::PixelInfo* SquareSubdivisionMeshV2::GetLeft(PixelInfo* p, int subdiv) const
{
	// p does not exist, it does not have the necessary neighbor,
	// or the neighbor lives at a subdivision level that is too low
	// Note that the left-neighbors are defined to be reciprocal with the right-neighbor,
	// therefore we must check the NEIGHBOR's subdivide level (and not p's)!
	if (!p || !p->LeftNbr || p->LeftNbr->SubdivideLevel < subdiv)
		return nullptr;
	else
	{
		if (p->LeftNbr->SubdivideLevel == subdiv)
		{
			// The sought-after neighbor is precisely one step away
			return p->LeftNbr;
		}
		else
		{
			// Here, necessarily p->LeftNbr->SubdivideLevel > subdiv
			// This means p is subdivided to a higher level than we want
			// In this case, we must travel over this finer grid to find the pixel we are looking for
			return GetLeft(GetLeft(p, subdiv + 1), subdiv + 1);
		}
	}
}

SquareSubdivisionMeshV2::PixelInfo* SquareSubdivisionMeshV2::GetRight(PixelInfo* p, int subdiv) const
{
	// p does not exist, it does not have the necessary neighbor,
	// or the neighbor lives at a subdivision level that is too low
	if (!p || !p->RightNbr || p->SubdivideLevel < subdiv)
		return nullptr;
	else
	{
		if (p->SubdivideLevel == subdiv)
		{
			// The sought-after neighbor is precisely one step away
			return p->RightNbr;
		}	
		else
		{
			// Here, necessarily p->SubdivideLevel > subdiv
			// This means p is subdivided to a higher level than we want
			// In this case, we must travel over this finer grid to find the pixel we are looking for
			return GetRight(GetRight(p, subdiv + 1), subdiv + 1);
		}
	}
}



// Helper function: subdivides the square with pixel m_ActivePixels[ind] in the upper-left corner,
// and add (up to) 5 new pixels in the integration queue accordingly
void SquareSubdivisionMeshV2::SubdivideAndQueue(largecounter ind)
{
	// The pixels are numbered as
	// 1 2 3
	// 4 5 6
	// 7 8 9
	// i.e. the initial square is given by (1, 3, 7, 9), and the new pixels are (2, 4, 5, 6, 8).
	PixelInfo* pixel1 = m_ActivePixels[ind];
	PixelInfo* pixel3 = pixel1->RightNbr;
	PixelInfo* pixel7 = pixel1->DownNbr;
	PixelInfo* pixel9 = pixel1->SEdiagNbr;

	// We are subdividing this pixel, so we increase the subdivision level
	int newsubdiv = pixel1->SubdivideLevel + 1;
	pixelcoord row{}, col{};


	//// PIXELS 1, 2, 4, 5: these are pixels who can subdivide again,
	// so we need to set their current subdivision level to newsubdiv

	// PIXEL 1
	// The pixel itself now has the new subdivision level
	pixel1->SubdivideLevel = newsubdiv;
	if (newsubdiv < m_MaxSubdivide)
		m_CurrentPixelUpdating.push_back(pixel1);

	// PIXEL 2
	// Does pixel 2 already exist? This could only be if it was created as part of a subdivided pixel
	// at subdivision level newsubdiv immediately above the current pixel
	PixelInfo* pixel2 = GetDown(GetRight(GetUp(pixel1, newsubdiv), newsubdiv), newsubdiv);
	if (!pixel2) // pixel does not exist yet, so we need to create it and integrate it
	{
		row = pixel1->Index[0];
		col = pixel1->Index[1] + ExpInt(2, m_MaxSubdivide - newsubdiv);
		m_AllPixels.push_front(std::make_unique<PixelInfo>(ScreenIndex{ row, col }, newsubdiv));
		pixel2 = m_AllPixels.front().get();
		m_CurrentPixelQueue.push_back(pixel2);
	}
	else 
	{
		// pixel already exists, so just set its subdivision level now
		// Note: pixel 2's correct subdivision level is always the one given by THIS subdivision (even if it had been created
		// by a subdivision of a different pixel)
		pixel2->SubdivideLevel = newsubdiv;
	}
	if (newsubdiv < m_MaxSubdivide)
		m_CurrentPixelUpdating.push_back(pixel2);


	// PIXEL 4
	// Does pixel 4 already exist?
	PixelInfo* pixel4 = GetRight(GetDown(GetLeft(pixel1, newsubdiv), newsubdiv), newsubdiv);
	if (!pixel4) // pixel does not exist yet, so we need to create it and integrate it
	{
		row = pixel1->Index[0] + ExpInt(2, m_MaxSubdivide - newsubdiv);
		col = pixel1->Index[1];
		m_AllPixels.push_front(std::make_unique<PixelInfo>(ScreenIndex{ row, col }, newsubdiv));
		pixel4 = m_AllPixels.front().get();
		m_CurrentPixelQueue.push_back(pixel4);
	}
	else
	{
		// pixel already exists, so just set its subdivision level now
		// Note: pixel 4's correct subdivision level is always the one given by THIS subdivision (even if it had been created
		// by a subdivision of a different pixel)
		pixel4->SubdivideLevel = newsubdiv;
	}
	if (newsubdiv < m_MaxSubdivide)
		m_CurrentPixelUpdating.push_back(pixel4);

	
	// PIXEL 5
	// Pixel 5 will never exist already!
	row = pixel1->Index[0] + ExpInt(2, m_MaxSubdivide - newsubdiv);
	col = pixel1->Index[1] + ExpInt(2, m_MaxSubdivide - newsubdiv);
	m_AllPixels.push_front(std::make_unique<PixelInfo>(ScreenIndex{ row, col }, newsubdiv));
	PixelInfo* pixel5 = m_AllPixels.front().get();
	m_CurrentPixelQueue.push_back(pixel5);
	if (newsubdiv < m_MaxSubdivide)
		m_CurrentPixelUpdating.push_back(pixel5);


	//// PIXELS 3, 7, 9: these pixels exist already and do NOT need new updating of subdivision level or right/down nbrs

	//// PIXELS 6, 8: These pixels could need to be created, they are created at subdivision level 0

	// PIXEL 6
	// Does pixel 6 already exist?
	PixelInfo* pixel6 = GetDown(pixel3, newsubdiv);
	if (!pixel6) // pixel does not exist yet, so we need to create it and integrate it
	{
		row = pixel1->Index[0] +  ExpInt(2, m_MaxSubdivide - newsubdiv);
		col = pixel1->Index[1] + 2 * ExpInt(2, m_MaxSubdivide - newsubdiv);
		m_AllPixels.push_front(std::make_unique<PixelInfo>(ScreenIndex{ row, col }, 0));
		pixel6 = m_AllPixels.front().get();
		m_CurrentPixelQueue.push_back(pixel6);
	}
	// no else to update subdivision level and no putting pixel 6 in m_CurrentPixelUpdating!

	// PIXEL 8
	// Does pixel 8 already exist?
	PixelInfo* pixel8 = GetRight(pixel7, newsubdiv);
	if (!pixel8) // pixel does not exist yet, so we need to create it and integrate it
	{
		row = pixel1->Index[0] + 2 * ExpInt(2, m_MaxSubdivide - newsubdiv);
		col = pixel1->Index[1] +  ExpInt(2, m_MaxSubdivide - newsubdiv);
		m_AllPixels.push_front(std::make_unique<PixelInfo>(ScreenIndex{ row, col }, 0));
		pixel8 = m_AllPixels.front().get();
		m_CurrentPixelQueue.push_back(pixel8);
	}
	// no else to update subdivision level and no putting pixel 8 in m_CurrentPixelUpdating!


	// Now all 9 pixels exist and have been put in the appropriate vectors (m_CurrentPixelQueue and/or m_CurrentPixelUpdating)
	// We only need to create the new neighbor relations between these pixels;
	// Remember: right/down/SEdiag neighbors are ONLY defined if the pixel is a pixel that can be subdivided;
	// and left/up neighbors are defined to be correct reciprocally with right/down relations
	// This means we only (re)define right/down/SEdiag neighbors for pixels 1, 2, 4, 5
	// Pixel 1
	pixel1->RightNbr = pixel2;
	pixel2->LeftNbr = pixel1;
	pixel1->DownNbr = pixel4;
	pixel4->UpNbr = pixel1;
	pixel1->SEdiagNbr = pixel5;
	// Pixel 2
	pixel2->RightNbr = pixel3;
	pixel3->LeftNbr = pixel2;
	pixel2->DownNbr = pixel5;
	pixel5->UpNbr = pixel2;
	pixel2->SEdiagNbr = pixel6;
	// Pixel 4
	pixel4->RightNbr = pixel5;
	pixel5->LeftNbr = pixel4;
	pixel4->DownNbr = pixel7;
	pixel7->UpNbr = pixel4;
	pixel4->SEdiagNbr = pixel8;
	// Pixel 5
	pixel5->RightNbr = pixel6;
	pixel6->LeftNbr = pixel5;
	pixel5->DownNbr = pixel8;
	pixel8->UpNbr = pixel5;
	pixel5->SEdiagNbr = pixel9;
}



// This function is called at the end of each integration iteration loop.
// We must wrap up the current iteration and initialize the next one.
void SquareSubdivisionMeshV2::EndCurrentLoop()
{
	///////////////////////////////////////////
	//// Wrap up of current iteration loop ////

	Utilities::Timer meshTimer;

	// First, we make sure all geodesics have indeed been integrated
	bool alldone{ true };
	for (largecounter i = 0; i < m_CurrentPixelQueueDone.size() && alldone; ++i)
	{
		if (!m_CurrentPixelQueueDone[i])
		{
			alldone = false;
		}
	}
	if (!alldone)
		ScreenOutput("Not all pixels have been integrated!", OutputLevel::Level_0_WARNING);

	m_PixelsIntegrated += static_cast<largecounter>(m_CurrentPixelQueue.size());

	// All pixels in CurrentPixelQueue have been integrated, so pixel queue is now empty
	m_CurrentPixelQueue.clear();
	m_CurrentPixelQueueDone.clear();

	ScreenOutput("Total integrated geodesic so far: " + std::to_string(m_PixelsIntegrated) + ".", OutputLevel::Level_2_SUBPROC);


	//////////////////////////////////////////
	//// Initializing next iteration loop ////

	ScreenOutput("Calculating pixels to subdivide next...", OutputLevel::Level_2_SUBPROC);

	// Now, we want to create a new pixel queue, but only if there are pixels left to integrate
	if (m_InfinitePixels || m_PixelsLeft > 0)
	{
		// Update all neighbors and weights of existing pixels
		UpdateAllWeights();
		// Now, m_CurrentPixelUpdating has also been cleared

		ScreenOutput("Selecting pixels for subdivision from " + std::to_string(m_ActivePixels.size()) + " active pixels...",
			OutputLevel::Level_3_ALLDETAIL);

		// UpdateWeights() has updated m_ActivePixels with the pixels that have weight > 0,
		// so this vector contains all possible candidates to subdivide. We only need to select the best candidates now.
		// Order the candidates according to how much we want to subdivide them (front is most important)
		auto Comp = [this](PixelInfo* p1, PixelInfo* p2) -> bool // returns true if ind1 is more important than ind2
		{
			if (p1->Weight > p2->Weight)
				return true;
			else if (p1->Weight == p2->Weight
				&& p1->SubdivideLevel < p2->SubdivideLevel)
				return true; // In the case of equal weight, give precedence to less-subdivided pixels
			else
				return false;
		};
		std::sort(m_ActivePixels.begin(), m_ActivePixels.end(), Comp);

		// Now we can actually subdivide the first m pixels
		ScreenOutput("Setting up subdivided pixels for integration...", OutputLevel::Level_3_ALLDETAIL);
		// At most, we will be creating 5x this number of new pixels to integrate
		m_CurrentPixelQueue.reserve(5 * m_IterationPixels);
		// Populate the queue with <=5*m_IterationPixels to integrate
		largecounter subdivlim = std::min(static_cast<largecounter>(m_ActivePixels.size()), m_IterationPixels);
		for (largecounter ind = 0; ind < subdivlim; ++ind)
		{
			SubdivideAndQueue(ind);
		}
		// Erase the elements that we have subdivided from the active pixels
		m_ActivePixels.erase(m_ActivePixels.begin(), m_ActivePixels.begin() + subdivlim);


		// If the total queue we have created is too large, truncate it
		// We choose to delete the last elements. These should be the least important in the queue due to the ordering process above.
		if (!m_InfinitePixels && m_CurrentPixelQueue.size() > m_PixelsLeft)
			m_CurrentPixelQueue.erase(m_CurrentPixelQueue.begin() + m_PixelsLeft, m_CurrentPixelQueue.end());

		// Queue is constructed now, make sure to subtract the pixels in the queue from the total we have left
		if (!m_InfinitePixels)
		{

			m_PixelsLeft -= static_cast<largecounter>(m_CurrentPixelQueue.size());
			ScreenOutput("Still max. " + std::to_string(m_PixelsLeft) +
				" left to integrate after the currently queued " + std::to_string(m_CurrentPixelQueue.size()) + ".",
				OutputLevel::Level_2_SUBPROC);
		}
		// Initialize m_CurrentPixelQueueDone
		m_CurrentPixelQueueDone = std::vector<bool>(m_CurrentPixelQueue.size(), false);
	}

	ScreenOutput("Done calculating next iteration of pixels (time taken: " + std::to_string(meshTimer.elapsed()) + "s).",
		OutputLevel::Level_2_SUBPROC);
	// Our queue is ready for integration now!
	// if no pixels are left to integrate, the queue will have remained empty (and IsFinished() will now return true)
	// (the same is true if we have not actually managed to "create" any new pixels in the subdivision process)
}