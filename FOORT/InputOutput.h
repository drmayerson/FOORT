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
	Level_0_WARNING = 0,	// Only warnings are outputted
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


constexpr int OutputHandler_All = -1;

class GeodesicOutputHandler
{
public:
	GeodesicOutputHandler(std::string FilePrefix, std::string TimeStamp, std::string FileExtension,
		std::vector<std::string> DiagNames, int nroutputstocache = OutputHandler_All, int geodperfile = OutputHandler_All,
		std::string firstlineinfo="");

	void NewGeodesicOutput(std::vector<std::string> theOutput);

	void OutputFinished();

private:
	void WriteCachedOutputToFile();

	void OpenForFirstTime(std::string filename);

	std::string GetFileName(int diagnr, int filenr) const;

	const std::string m_FilePrefix;
	const std::string m_TimeStamp;
	const std::string m_FileExtension;
	const std::vector<std::string> m_DiagNames;

	const bool m_PrintFirstLineInfo;
	const std::string m_FirstLineInfoString;

	const int m_nrOutputsToCache{};
	const int m_nrGeodesicsPerFile{};

	bool m_WriteToConsole{ false };

	int m_CurrentGeodesicsInFile{ 0 };
	int m_CurrentFullFiles{ 0 };


	std::vector<std::vector<std::string>> m_AllCachedData{};
};







#endif
