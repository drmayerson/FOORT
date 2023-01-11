#ifndef _FOORT_INPUTOUTPUT_H
#define _FOORT_INPUTOUTPUT_H

///////////////////////////////////////////////////////////////////////////////////////////////
////// INPUTOUTPUT.H
////// Declarations of functions & classes that deal with the output to files and to the screen
////// Definitions in InputOutput.cpp
///////////////////////////////////////////////////////////////////////////////////////////////

#include "Geometry.h" // for basic tensor objects

#include <string_view> // std::string_view
#include <iostream> // needed for file and console output
#include <fstream> // needed for file ouput
#include <string> // std::string used in various places
#include <vector> // needed to create vectors of strings


//////////////////////
//// SCREENOUTPUT ////

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

// Set the output level used; note: the output level itself is a static variable in InputOutput.cpp
void SetOutputLevel(OutputLevel theLvl);

// Outputs line to screen console), contingent on it being allowed by the set outputlevel
void ScreenOutput(std::string_view theOutput, OutputLevel lvl = OutputLevel::Level_3_ALLDETAIL, bool newLine = true);

// Set and Get for the loop message frequency (messages indicating progress during each integration loop)
void SetLoopMessageFrequency(largecounter thefreq);
largecounter GetLoopMessageFrequency();


/////////////////////
//// FILE OUTPUT ////
// GeodesicOutputHandler declaration

// GeodesicOutputHandler handles all of the output to file.
// It gets passed all of the output strings for every Geodesic, it then
// stores this data until it eventually writes all data to the appropriate files
class GeodesicOutputHandler
{
public:
	// No default constructor possible
	GeodesicOutputHandler() = delete;
	// Constructor must pass the following strings that are used to construct the file names of the output file:
	// FilePrefix, TimeStamp, FileExtension, and a vector of strings DiagNames (the names of each of the Diagnostics that will
	// be outputting). It must also specify how many outputs to cache before outputting to a file, and
	// how many geodesics are allowed per file created
	GeodesicOutputHandler(std::string FilePrefix, std::string TimeStamp, std::string FileExtension,
		std::vector<std::string> DiagNames,
		largecounter nroutputstocache = LARGECOUNTER_MAX-1, // note -1,
										// since we will actually cache one more then this number before outputting everything
		largecounter geodperfile = LARGECOUNTER_MAX,
		std::string firstlineinfo="");

	// This tells the OutputHandler to prepare for this many geodesic outputs to arrive;
	// the internal state needs to be prepared such that they can come in without providing a data race
	void PrepareForOutput(largecounter nrOutputToCome);

	// A new vector of output strings from a (single) Geodesic;
	// the length of the vector should be m_DiagNames.size()+1, since the first entry
	// is the screen index
	// NOTE: this procedure needs to be thread-safe!
	void NewGeodesicOutput(largecounter index, std::vector<std::string> theOutput);

	// Calling this indicates that there is no further output to be expected;
	// this means we will write all remaining cached output to file
	void OutputFinished();

	// Returns full description string of output handler
	std::string getFullDescriptionStr() const;

private:
	// Helper function: write everything that is cached to file now (clear the cache)
	void WriteCachedOutputToFile();

	// Helper function: open the file with name filename for the first time, preparing it to write
	// This will effectively clear this file of any pre-existing content.
	void OpenForFirstTime(const std::string& filename);

	// Helper function: return the full file name for the n-th output file
	// for the Diagnostic diagnr (this is an entry in m_DiagNames)
	std::string GetFileName(int diagnr, unsigned short filenr) const;

	// The const strings that are used to construct the output file names
	const std::string m_FilePrefix;
	const std::string m_TimeStamp;
	const std::string m_FileExtension;
	const std::vector<std::string> m_DiagNames;

	// const variables that control whether we write a descriptive first line in
	// every output file or not, and what that first line is
	const bool m_PrintFirstLineInfo;
	const std::string m_FirstLineInfoString;

	// consts setting the maximum number of outputs that can be cached before writing output to file,
	// and the max number of geodesics to store in a file
	const largecounter m_nrOutputsToCache{};
	const largecounter m_nrGeodesicsPerFile{};

	// If this is false, then we are writing to file(s). At any time, if a file I/O error occurs,
	// the output handler switches to outputting everything to the console
	bool m_WriteToConsole{ false };

	// How many outputs are already cached in m_nrOutputsToCache before the current iteration of output
	largecounter m_PrevCached{ 0 };

	// The number of geodesics already written to the current file
	// (once this hits m_nrGeodesicsPerFile, this file is full)
	largecounter m_CurrentGeodesicsInFile{ 0 };

	// The current counter of completely full files (this is kept track of
	// so that it knows what the next file to write output to is)
	// (We had better not have more than 60k files!)
	unsigned short m_CurrentFullFiles{ 0 };

	// Cached data that has not been written to a file yet
	// (once this hits a size of > m_nrOutputsToCache,
	// this must be written to file(s))
	std::vector<std::vector<std::string>> m_AllCachedData{};
};

#endif
