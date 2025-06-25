#include "InputOutput.h" // We are defining functions from here

#include <algorithm>  // needed for std::min etc
#include <filesystem> // needed for std::filesystem::create_directories

/**
 * @file InputOutput.cpp
 * @author Daniel R. Mayerson
 * @version 0.1
 * @date 2025-01-14
 * @copyright Copyright (c) 2025
 */

// Screen output functions

//! This is the output level; default is 1. Note: variable only accessible in this code file!
static OutputLevel theOutputLevel{OutputLevel::Level_1_PROC};

//! Frequency of messages during each integration loop.  Note: variable only accessible in this code file!
static largecounter theLoopMessageFrequency{LARGECOUNTER_MAX};

/**
 * @brief Set the Output Level
 *
 * @param theLvl The desired level
 */
void SetOutputLevel(OutputLevel theLvl)
{
	theOutputLevel = theLvl;
}

/**
 * @brief Set the Loop Message Frequency
 *
 * @param thefreq the desired frequency
 */
void SetLoopMessageFrequency(largecounter thefreq)
{
	theLoopMessageFrequency = thefreq;
}

/**
 * @brief Get the Loop Message Frequency during integration loops
 *
 * @return largecounter
 */
largecounter GetLoopMessageFrequency()
{
	return theLoopMessageFrequency;
}

/**
 * @brief Outputs line to screen console, contingent on it being allowed by the set outputlevel
 * @details Defaults are lvl = OutputLevel::Level_3_ALLDETAIL and newLine = true
 * @param theOutput output to be put on the screen
 * @param lvl the output level
 * @param newLine indicates whether we want a new line after the output
 */
void ScreenOutput(std::string_view theOutput, OutputLevel lvl, bool newLine)
{
	if (static_cast<int>(lvl) <= static_cast<int>(theOutputLevel)) // Output is allowed at current set level
	{
		if (lvl == OutputLevel::Level_0_WARNING) // Print extra prefix to message for warnings
		{
			std::cout << "WARNING: ";
		}
		if (lvl == OutputLevel::Level_4_DEBUG) // Print extra prefix to message for debug statements
		{
			std::cout << "DEBUG MSG: ";
		}
		std::cout << theOutput;
		if (newLine) // We want the line to end after the message
		{
			std::cout << '\n';
		}
	}
}

// GeodesicOutputHandler functions

/**
 * @brief Construct a new Geodesic Output Handler object and initialize all const member variables
 *
 * @param FilePrefix file prefix
 * @param TimeStamp time stamp
 * @param FileExtension file extensions
 * @param DiagNames diagnostics names
 * @param nroutputstocache number of geodesics to put to the cache
 * @param geodperfile max number of geodesics per output file
 * @param firstlineinfo the info to print on the first line
 */
GeodesicOutputHandler::GeodesicOutputHandler(std::string FilePrefix, std::string TimeStamp, std::string FileExtension,
											 std::vector<std::string> DiagNames, largecounter nroutputstocache, largecounter geodperfile, std::string firstlineinfo) : m_FilePrefix{FilePrefix}, m_TimeStamp{TimeStamp}, m_FileExtension{FileExtension}, m_DiagNames{DiagNames},
																																									   // Make sure that we only cache up to the max amount that fits in largecounter
																																									   // OR, if smaller, the max amount of elements that can be reserved in the cache vector
																																									   m_nrOutputsToCache{static_cast<largecounter>(std::min({static_cast<size_t>(nroutputstocache),
																																																							  m_AllCachedData.max_size() - 1,
																																																							  static_cast<size_t>(LARGECOUNTER_MAX - 1)}))},
																																									   m_nrGeodesicsPerFile{geodperfile}, m_PrintFirstLineInfo{firstlineinfo != ""},
																																									   m_FirstLineInfoString{firstlineinfo}
{
	// If no prefix has been set, or we are allowed zero geodesics per file, then we necessarily output to the console
	if (m_FilePrefix == "" || m_nrGeodesicsPerFile == 0)
		m_WriteToConsole = true;
}

/**
 * @brief full description string getter for the geodesic output handler
 *
 * @return std::string
 */
std::string GeodesicOutputHandler::getFullDescriptionStr() const
{
	// Descriptive string with all options
	if (!m_WriteToConsole)
	{
		return "Output Handler: Basic (value diagnostic) file name: " + GetFileName(0, 1) + ", caching outputs: " + std::to_string(m_nrOutputsToCache) + ", geodesics per file: " + std::to_string(m_nrGeodesicsPerFile) + ", printing first line info: " + std::to_string(m_PrintFirstLineInfo);
	}
	else
	{
		return "Output Handler: Writing to console, caching outputs: " + std::to_string(m_nrOutputsToCache);
	}
}

/**
 * @brief prepare the cache for output
 *
 * @param nrOutputToCome number of outputs to come into the cache
 */
void GeodesicOutputHandler::PrepareForOutput(largecounter nrOutputToCome)
{
	// If the output that is coming will put us over the caching limit, first write the cached data to file
	if (m_AllCachedData.size() + nrOutputToCome > m_nrOutputsToCache)
	{
		WriteCachedOutputToFile();
	}

	// This many outputs are already stored in the cached data, so we need to offset the incoming data by this much
	m_PrevCached = static_cast<largecounter>(m_AllCachedData.size());

	// We prepare our vector of cache data to receive the output: we must create dummy vectors of strings
	// so that the received output will simply overwrite these (instead of placing a new vector of strings into
	// m_AllCachedData, which would introduce data races)
	m_AllCachedData.insert(m_AllCachedData.end(), nrOutputToCome, std::vector<std::string>{});
}

/**
 * @brief
 *
 * @param index
 * @param theOutput
 */
void GeodesicOutputHandler::NewGeodesicOutput(largecounter index, std::vector<std::string> theOutput)
{
	// NOTE: this must be thread-safe! Indeed, we are only overwriting an existing element of m_AllCachedData

	// We put this current output in the cached data
	// Note the offset by m_PrevCached
	// Move semantics to avoid copying the entire vector of string
	m_AllCachedData[m_PrevCached + index] = std::move(theOutput);
}

/**
 * @brief There is no more output, so we write anything that is cached to file to clean up and finalize
 */
void GeodesicOutputHandler::OutputFinished()
{
	WriteCachedOutputToFile();
}

/**
 * @brief Write cached output to a file
 */
void GeodesicOutputHandler::WriteCachedOutputToFile()
{
	// Check if there is anything to do
	if (m_AllCachedData.size() == 0)
		return;

	if (!m_WriteToConsole) // We are writing to files
	{
		ScreenOutput("Writing cached geodesic output to file(s)...", OutputLevel::Level_2_SUBPROC);

		// Check if we will be breaking the cached output into multiple files
		unsigned short nrfiles{1};
		// Note that the constructor has checked that indeed m_nrGeodesicsPerFile > 0
		while (m_CurrentGeodesicsInFile + m_AllCachedData.size() > nrfiles * m_nrGeodesicsPerFile)
			++nrfiles;

		// Keeps track of the current file we are working in; note that this will be offset by the number of
		// full files when specifying the true file number to write to
		unsigned short curfile{1};
		// The next geodesic (index in m_AllCachedData) that we need to output
		largecounter curgeod{0};
		// The number of geodesics stored in the last file we will open for output
		largecounter lastfilecount{0};
		// We are sure that the number of diagnostics fits in an int!
		// Note that the first element of each output vector is the screen index
		const int nrdiags{static_cast<int>(m_AllCachedData[0].size()) - 1};

		// Are we exactly at a point where we need to start a new file for the first file we write to?
		// If so, open each of the necessary new output files for the first time
		if (m_CurrentGeodesicsInFile == 0)
		{
			// Starting a new file (for each diagnostic), so open it for the first time
			for (int i = 0; i < nrdiags && !m_WriteToConsole; ++i)
			{
				OpenForFirstTime(GetFileName(i, m_CurrentFullFiles + curfile));
			}
		}

		// The m_WriteToConsole check is because any file I/O error will trigger setting this to true
		// We now loop through all of the files we need to write to
		while (curfile <= nrfiles && !m_WriteToConsole)
		{
			largecounter loopmax{static_cast<largecounter>(m_AllCachedData.size())};
			if (curfile == 1)
			{
				// if this is the first file we are writing to, then there could already be geodesics written
				// to this file which we need to take into account
				// (note that then automatically curgeod == 0)
				loopmax = std::min(static_cast<largecounter>(m_AllCachedData.size()), m_nrGeodesicsPerFile - m_CurrentGeodesicsInFile);
			}
			else
			{
				// If this is not the first file we are writing to, then see if we will write all geodesics to this file or not
				loopmax = std::min(static_cast<largecounter>(m_AllCachedData.size()) - curgeod, m_nrGeodesicsPerFile);
			}

			// loop through each diagnostic
			for (int curdiag = 0; curdiag < nrdiags; ++curdiag)
			{
				// open appropriate file for appending
				std::string outputfile{GetFileName(curdiag, m_CurrentFullFiles + curfile)};
				std::ofstream outf{outputfile, std::ios::out | std::ios::app};

				// If something goes wrong in opening the file,
				// then write to console from now on
				if (!outf)
				{
					ScreenOutput("Output file error! Could not open " + GetFileName(curdiag, m_CurrentFullFiles + curfile) + ". Will write rest of output to console.", OutputLevel::Level_0_WARNING);
					m_WriteToConsole = true;
				}
				else
				{
					// All went well opening the file. Output the current diagnostic data of the current diagnostic
					// for all geodesics that go in this file
					for (largecounter j = curgeod; j < curgeod + loopmax; ++j)
					{
						// Output pixel and then diagnostic data
						outf << m_AllCachedData[j][0] << " " << m_AllCachedData[j][curdiag + 1] << "\n";
					}

					// We are done with this diagnostic file, close file
					outf.close();
				}
			}

			// We have now filled up the current file (or we have outputted all cached data already)
			++curfile;
			if (curfile <= nrfiles) // Is there still a file to be (partially) written?
			{
				// We have outputted another loopmax geodesics, so increment curgeod accordingly
				curgeod += loopmax;
				// We will be starting a new file (for each diagnostic), so open it for the first time
				for (int i = 0; i < nrdiags && !m_WriteToConsole; ++i)
				{
					OpenForFirstTime(GetFileName(i, m_CurrentFullFiles + curfile));
				}
			}
			else // curfile > nrfiles, so we just did the last file
			{
				// This is how many geodesics went into the last file
				lastfilecount = loopmax;
			}
			// We are now done with one iteration of the loop; the next iteration
			// will write to the next file (for each geodesic). Note that we have prepared
			// this next file (for each geodesic) for writing by opening it already for the first time.
		} // end while

		// We are done writing all the output to files. Now, we update the internal counters
		// We have filled up entirely nrfiles-1 files for sure.
		m_CurrentFullFiles += nrfiles - 1;
		// The last file may have been exactly filled, check this
		if (lastfilecount == m_nrGeodesicsPerFile)
		{
			// If the last file was exactly filled, we have one more full file, and the next file to write to
			// will be empty
			++m_CurrentFullFiles;
			lastfilecount = 0;
		}
		// Set how many geodesics have already been written to in the current (non-full) file
		// If nrfiles == 1, then we have written more into the already existing file, so we increment
		// m_CurrentGeodesicsInFile by lastfilecount; otherwise m_CurrentGeodesicsInFile is equal to the lastfilecount
		if (nrfiles == 1)
			m_CurrentGeodesicsInFile += lastfilecount;
		else
			m_CurrentGeodesicsInFile = lastfilecount;

		ScreenOutput("Done writing cached geodesic output to file(s).", OutputLevel::Level_2_SUBPROC);
	} // end if (!m_WriteToConsole)

	if (m_WriteToConsole) // write everything to console; note this is not an else from the previous if since in the previous if,
						  // we may encounter file I/O problems that sets this to true in the if block above
	{
		for (int i = 0; i < m_AllCachedData.size(); ++i)
		{
			// write output line to console
			for (int j = 0; j < m_AllCachedData[i].size(); ++j)
				ScreenOutput(m_AllCachedData[i][j], OutputLevel::Level_1_PROC);
		}
	}

	// Whether we have written all output to file or to console, in any case we have outputted all cached data,
	// so empty the cache now
	m_AllCachedData.clear();
}

/**
 * @brief Get the file name based on the various parts of the file name stored in the member variables.
 *
 * @param diagnr
 * @param filenr
 * @return std::string
 */
std::string GeodesicOutputHandler::GetFileName(int diagnr, unsigned short filenr) const
{
	if (m_WriteToConsole)
		ScreenOutput("Should not be getting a file name if we are writing to console!", OutputLevel::Level_0_WARNING);

	std::string FullFileName{m_FilePrefix + "_"};

	if (m_TimeStamp != "")
		FullFileName += m_TimeStamp + "_";

	FullFileName += m_DiagNames[diagnr];

	if (filenr > 1) // only output the number of the file if it is not the first file
	{
		FullFileName += "_" + std::to_string(filenr);
	}

	if (m_FileExtension != "") // if there is not extension, we also don't want the trailing .
		FullFileName += "." + m_FileExtension;

	return FullFileName;
}

/**
 * @brief We check here to see if the files are being put in a (sub)directory;
 * if so, we create the directory/directories first to make sure creating/opening the file
 * will succeed.
 *
 * @param filename Filename to be found / created
 */
void GeodesicOutputHandler::OpenForFirstTime(const std::string &filename)
{
	auto pos = filename.find_last_of("/");
	if (pos != std::string::npos) // there is at least one slash, so files are in a (sub)directory
	{
		// This creates all necessary directories in this structure
		// If the directory already exists, it does nothing
		std::filesystem::create_directories(filename.substr(0, pos));
	}

	// Open the file, effectively overwriting the file
	std::ofstream outf{filename, std::ios::out | std::ios::trunc};

	if (!outf) // Trigger writing to console if failed to open file
	{
		ScreenOutput("Output file error! Could not open " + filename + ". Will write rest of output to console.", OutputLevel::Level_0_WARNING);
		m_WriteToConsole = true;
	}
	else
	{
		// write first line info in the file; it is then prepared for geodesic output
		if (m_PrintFirstLineInfo)
			outf << m_FirstLineInfoString << "\n";

		// Close the file
		outf.close();
	}
}
