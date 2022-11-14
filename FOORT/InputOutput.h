#ifndef _FOORT_INPUTOUTPUT_H
#define _FOORT_INPUTOUTPUT_H

#include<string_view>
#include<iostream>
#include<string>
#include<vector>
#include<fstream>




// Determines at what priority level the output is generated to the console.
enum class OutputLevel
{
	Level_0_NONE = 0,		// No output at all
	Level_1_PROC = 1,		// Coarsest level output; only the major procedures produce output
	Level_2_SUBPROC = 2,	// Subprocedures can also produce output
	Level_3_ALLDETAIL = 3,	// Finest level output; all details are shown
	Level_4_DEBUG = 4,		// Finest level output AND debug messages as well

	MaxLevel				// (unused)
};

// Set the output level
void SetOutputLevel(OutputLevel theLvl);

// Outputs line to screen console), contingent on it being allowed by the set outputlevel
void ScreenOutput(std::string_view theOutput, OutputLevel lvl = OutputLevel::Level_3_ALLDETAIL, bool newLine = true);


constexpr int OutputHandler_CacheAll = -1;

class GeodesicOutputHandler
{
public:
	GeodesicOutputHandler(std::string outputfile,int nroutputstocache=OutputHandler_CacheAll) :
		m_OutputFile{ outputfile }, m_nrOutputsToCache{ nroutputstocache }
	{
		if (m_nrOutputsToCache>0)
			m_AllCachedData.reserve(m_nrOutputsToCache+1);

		// check to see if we indeed can open the file and write to it
		// std::ios::trunc makes the new file empty
		if (m_OutputFile != "")
		{
			std::ofstream outf{ m_OutputFile, std::ios::out | std::ios::trunc };
			if (!outf)
			{
				ScreenOutput("Error opening file " + m_OutputFile + " for writing output. Will output to console instead.",
					OutputLevel::Level_0_NONE);
				m_OutputFile = ""; // Will write to console
			}
			else
			{
				outf.close();
			}
		}
	}

	void NewGeodesicOutput(std::string theOutput);

	void OutputFinished();

private:
	void WriteCachedOutputToFile();

	int m_nrOutputsToCache{};
	std::string m_OutputFile{};
	std::vector<std::string> m_AllCachedData{};
};







#endif
