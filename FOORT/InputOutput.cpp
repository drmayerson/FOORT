#include"InputOutput.h"
#include<fstream>
#include<cassert>

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


void GeodesicOutputHandler::NewGeodesicOutput(std::string theOutput)
{
	m_AllCachedData.push_back(theOutput);
	if (m_nrOutputsToCache != OutputHandler_CacheAll && m_nrOutputsToCache < m_AllCachedData.size())
		WriteCachedOutputToFile();
}

void GeodesicOutputHandler::OutputFinished()
{
	WriteCachedOutputToFile();
}

void GeodesicOutputHandler::WriteCachedOutputToFile()
{
	bool writetoconsole = (m_OutputFile == "");

	if (!writetoconsole)
	{
		// open file for appending
		std::ofstream outf{ m_OutputFile, std::ios::out | std::ios::app };

		if (!outf)
			assert(false &&
				"Output file error! This should not happen (output file should have been checked when GeodesicOutputHandler created)!");

		for (int i=0; i<m_AllCachedData.size();i++)
		{
			// write output line to file
			outf << m_AllCachedData[i] << "\n";
		}

		// close file
		outf.close();
	}
	else
	{
		for (int i = 0; i < m_AllCachedData.size(); i++)
		{
			// write output line to console
			ScreenOutput(m_AllCachedData[i], OutputLevel::Level_0_NONE);
		}
	}
	
	
	// Clean up cache now
	m_AllCachedData.clear();
	m_AllCachedData.reserve(m_nrOutputsToCache + 1);
}
