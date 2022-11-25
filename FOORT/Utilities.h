#ifndef _FOORT_UTILITIES_H
#define _FOORT_UTILITIES_H

#include "Metric.h"
#include "Geodesic.h"
#include "Diagnostics.h"
#include "Terminations.h"
#include "ViewScreen.h"
#include "Integrators.h"

#include <chrono>
#include <string>
#include <vector>

namespace Utilities
{
    class Timer
    {
    private:
        // Type aliases to make accessing nested type easier
        using Clock = std::chrono::steady_clock;
        using Second = std::chrono::duration<double, std::ratio<1> >;

        std::chrono::time_point<Clock> m_beg{ Clock::now() };

    public:
        void reset();

        double elapsed() const;
    };

    std::string GetTimeStampString();

    std::vector<std::string> GetDiagStrings(DiagBitflag alldiags, DiagBitflag valdiag);

    std::string GetFirstLineInfoString(const Metric* theMetric, const Source* theSource,
        DiagBitflag alldiags, DiagBitflag valdiag, TermBitflag allterms, const ViewScreen* theView,
        GeodesicIntegratorFunc theIntegrator);
}

#endif
