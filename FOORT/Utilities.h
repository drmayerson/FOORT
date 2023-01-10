#ifndef _FOORT_UTILITIES_H
#define _FOORT_UTILITIES_H

///////////////////////////////////////////////////////////////////////////////////////
////// UTILITIES.H
////// Declarations of various utility functions, in a Utilities namespace
////// All definitions in Utilities.cpp
///////////////////////////////////////////////////////////////////////////////////////

// We use Metric, Diagnostic, Termination, Geodesic, ViewScreen, and Integrator declarations here
#include "Metric.h"
#include "Diagnostics.h"
#include "Terminations.h"
#include "Geodesic.h"
#include "ViewScreen.h"
#include "Integrators.h"

#include <chrono> // for timer functionality
#include <string> // for strings
#include <vector> // for std::vector

// Namespace that contains our utility functions
namespace Utilities
{
    // Timer class to keep track of elapsed time
    class Timer
    {
    private:
        // Type aliases to make accessing nested type easier
        using Clock = std::chrono::steady_clock;
        using Second = std::chrono::duration<double, std::ratio<1> >;

        // begin time
        std::chrono::time_point<Clock> m_beg{ Clock::now() };

    public:
        // reset begin time
        void reset();

        // returns time elapsed since begin time
        double elapsed() const;
    };

    // Returns a string of the current time (in a format that can be used to append to file names)
    std::string GetTimeStampString();

    // Helper function to get all Diagnostic Names (for outputting to files)
    std::vector<std::string> GetDiagNameStrings(DiagBitflag alldiags, DiagBitflag valdiag);

    // This returns the full string to be written to every output file as its first line
    // It contains information about all the settings used to produce the output
    std::string GetFirstLineInfoString(const Metric* theMetric, const Source* theSource,
        DiagBitflag alldiags, DiagBitflag valdiag, TermBitflag allterms, const ViewScreen* theView);
}

#endif
