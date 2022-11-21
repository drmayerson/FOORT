#include"InputOutput.h"
#include<fstream>
#include<algorithm>

// This is the output level; default is 1. Note: variable only accessible in this code file!
static OutputLevel theOutputLevel{ OutputLevel::Level_1_PROC };

// Set the output level
void SetOutputLevel(OutputLevel theLvl)
{
	theOutputLevel = theLvl;
}

// Outputs line to screen console), contingent on it being allowed by the set outputlevel
void ScreenOutput(std::string_view theOutput, OutputLevel lvl, bool newLine)
{
	if (static_cast<int>(lvl) <= static_cast<int>(theOutputLevel))	// Output is allowed at current set level
	{
		if (lvl == OutputLevel::Level_0_WARNING)	// Print extra prefix to message for warnings
		{
			std::cout << "WARNING: ";
		}
		if (lvl == OutputLevel::Level_4_DEBUG)	// Print extra prefix to message for debug statements
		{
			std::cout << "DEBUG MSG: ";
		}
		std::cout << theOutput;
		if (newLine)				// We want the line to end after the message
		{
			std::cout << '\n';
		}
	}
}

GeodesicOutputHandler::GeodesicOutputHandler(std::string FilePrefix, std::string TimeStamp, std::string FileExtension,
	std::vector<std::string> DiagNames, int nroutputstocache, int geodperfile, std::string firstlineinfo) :
	m_FilePrefix {FilePrefix}, m_TimeStamp{TimeStamp}, m_FileExtension{FileExtension}, m_DiagNames{DiagNames},
	m_nrOutputsToCache{ nroutputstocache }, m_nrGeodesicsPerFile{ geodperfile }, m_PrintFirstLineInfo{ firstlineinfo != "" },
	m_FirstLineInfoString{ firstlineinfo }
{
	if (m_FilePrefix == "")
		m_WriteToConsole = true;

	if (m_nrOutputsToCache > 0)
		m_AllCachedData.reserve(m_nrOutputsToCache + 1);
}


void GeodesicOutputHandler::NewGeodesicOutput(std::vector<std::string> theOutput)
{
	m_AllCachedData.push_back(theOutput);
	if (m_nrOutputsToCache != OutputHandler_All && m_nrOutputsToCache < m_AllCachedData.size())
		WriteCachedOutputToFile();
}

void GeodesicOutputHandler::OutputFinished()
{
	WriteCachedOutputToFile();
}

void GeodesicOutputHandler::WriteCachedOutputToFile()
{
	if (!m_WriteToConsole)
	{
		ScreenOutput("Writing cached geodesic output to file(s)...", OutputLevel::Level_2_SUBPROC);

		// Check if we will be breaking the cached output into multiple files
		int nrfiles{ 1 };
		if (m_nrGeodesicsPerFile > 0)
		{
			while (m_AllCachedData.size() > nrfiles * m_nrGeodesicsPerFile)
				++nrfiles;
		}
		int curfile{ 1 };
		int curgeod{ 0 };
		int lastfilecount{ 0 };
		const int nrdiags{ static_cast<int>(m_AllCachedData[0].size()) - 1 };
		
		if (m_CurrentGeodesicsInFile == 0)
		{
			// Starting a new file (for each diagnostic), so open it for the first time
			for (int i = 0; i < nrdiags && !m_WriteToConsole; ++i)
			{
				std::string outputfile{ GetFileName(i,m_CurrentFullFiles + curfile) };
				OpenForFirstTime(outputfile);
			}
		}

		while (curfile <= nrfiles && !m_WriteToConsole)
		{
			int loopmax{static_cast<int>(m_AllCachedData.size())};
			if (m_nrGeodesicsPerFile > 0)
			{
				if (curfile == 1)
					loopmax = std::min(static_cast<int>(m_AllCachedData.size()), m_nrGeodesicsPerFile - m_CurrentGeodesicsInFile);
				else
					loopmax = std::min(static_cast<int>(m_AllCachedData.size()) - curgeod, m_nrGeodesicsPerFile);
			}

			// loop through each diagnostic
			for (int curdiag = 0; curdiag < nrdiags; ++curdiag)
			{ 
				std::string outputfile{ GetFileName(curdiag, m_CurrentFullFiles + curfile) };
				// open file for appending
				std::ofstream outf{ outputfile, std::ios::out | std::ios::app };

				if (!outf)
				{
					ScreenOutput("Output file error! Could not open " + GetFileName(curdiag, m_CurrentFullFiles + curfile)
						+ ". Will write rest of output to console.", OutputLevel::Level_0_WARNING);
					m_WriteToConsole = true;
				}
				else
				{
					for (int j = curgeod; j < curgeod + loopmax; ++j)
					{
						// Output pixel and then diagnostic data
						outf << m_AllCachedData[j][0] << " " << m_AllCachedData[j][curdiag+1] << "\n";
					}

					// close file
					outf.close();
				}
			}

			++curfile;
			if (curfile <= nrfiles)
			{
				curgeod += loopmax;
				// Starting a new file (for each diagnostic), so open it for the first time
				for (int i = 0; i < nrdiags && !m_WriteToConsole; ++i)
				{
					std::string outputfile{ GetFileName(i,m_CurrentFullFiles + curfile) };
					OpenForFirstTime(outputfile);
				}
			}
			else // curfile > nrfiles, so we just did the last file
			{
				lastfilecount = loopmax;
			}

		} // end while

		// Fill up files
		m_CurrentFullFiles += nrfiles - 1;
		// Check if last file is actually just exactly full
		if (lastfilecount == m_nrGeodesicsPerFile)
		{
			++m_CurrentFullFiles;
			lastfilecount = 0;
		}
		m_CurrentGeodesicsInFile = lastfilecount;

		ScreenOutput("Done writing cached geodesic output to file(s).", OutputLevel::Level_2_SUBPROC);
	} // end if (!m_WriteToConsole)

	if (m_WriteToConsole)	// write everything to console; note this is not an else from the previous if since in the previous if,
							// we may encounter problems that sets this to true
	{
		for (int i = 0; i < m_AllCachedData.size(); ++i)
		{
			// write output line to console
			for (int j = 0; j  < m_AllCachedData[i].size(); ++j)
			ScreenOutput(m_AllCachedData[i][j], OutputLevel::Level_1_PROC);
		}
	}
	
	// Clean up cache now
	m_AllCachedData.clear();
	m_AllCachedData.reserve(m_nrOutputsToCache + 1);
}

std::string GeodesicOutputHandler::GetFileName(int diagnr, int filenr) const
{
	if (m_WriteToConsole)
		ScreenOutput("Should not be getting a file name if we are writing to console!", OutputLevel::Level_0_WARNING);

	std::string FullFileName{ m_FilePrefix + "_"};

	if (m_TimeStamp != "")
		FullFileName += m_TimeStamp + "_";

	FullFileName += m_DiagNames[diagnr];

	if (filenr > 1)
	{
		FullFileName += "_" + std::to_string(filenr);
	}

	if (m_FileExtension != "")
		FullFileName += "." + m_FileExtension;

	return FullFileName;
}

void GeodesicOutputHandler::OpenForFirstTime(std::string filename)
{
	std::ofstream outf{ filename, std::ios::out | std::ios::trunc };
	if (!outf)
	{
		ScreenOutput("Output file error! Could not open " + filename
			+ ". Will write rest of output to console.", OutputLevel::Level_0_WARNING);
		m_WriteToConsole = true;
	}
	else
	{
		// write first line info
		if (m_PrintFirstLineInfo)
			outf << m_FirstLineInfoString << "\n";

		outf.close();
	}
}
