#include <iostream>
#include<chrono>

#include "Geometry.h"
#include"Metrics_all.h"
#include"AllTests.h"

#include<variant>
#include<random>
#include<array>
#include<string>

#include<omp.h>


constexpr int nrtests = 5;
constexpr int nrouterloops = 100;

int main()
{
    std::vector<std::vector<std::string>> vecvecstring{};

    Timer thetimer;

#pragma omp parallel // start up threads!
    {

#pragma omp single
        {
            vecvecstring.clear();
            thetimer.reset();
        }
#pragma omp for
        for (unsigned int i = 0; i < 30000000; ++i)
        {
#pragma omp critical
            {
                vecvecstring.push_back(std::vector<std::string>({ "test", std::to_string(i) }));
            }
        }
#pragma omp single
        {
            std::cout << std::to_string(thetimer.elapsed()) << "s for pushing back 30M\n";
        }

#pragma omp single
        {
            vecvecstring.clear();
            thetimer.reset();
            vecvecstring.insert(vecvecstring.end(), 30000000, std::vector<std::string>{});
        }
#pragma omp for
        for (unsigned int i = 0; i < 30000000; ++i)
        {
            vecvecstring[i] = std::vector<std::string>({ "test", std::to_string(i) });
        }
#pragma omp single
        {
            std::cout << std::to_string(thetimer.elapsed()) << "s for pre-allocating 30M\n";
        }


#pragma omp single
        {

            vecvecstring.clear();
            thetimer.reset();
            for (unsigned int i = 0; i < 30000000; ++i)
            {
                vecvecstring.push_back(std::vector<std::string>({ "test", std::to_string(i) }));
            }
            std::cout << std::to_string(thetimer.elapsed()) << "s for pushing back 30M single thread\n";
        }


#pragma omp single
        {
            vecvecstring.clear();
            thetimer.reset();
            vecvecstring.insert(vecvecstring.end(), 30000000, std::vector<std::string>{});
            for (unsigned int i = 0; i < 30000000; ++i)
            {
                vecvecstring[i] = std::vector<std::string>({ "test", std::to_string(i) });
            }
            std::cout << std::to_string(thetimer.elapsed()) << "s for pre-allocating 30M\n";
        }



    }


    return 0;
}

int main_calls()
{
    // INITIALIZATION
    /////////////////

    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> rvals(2.0, 10.0), thetavals(0.1, 0.8), phivals(0.0, 6.0), aval(0.0, 1.0);

    std::cout << "Taking random a for Kerr: ";
    real thea = aval(gen);
    std::cout << thea << "\n";

    std::cout << "Populating array of " << nrpts << " random Points... ";
    std::vector<Point> testarray{};
    testarray.reserve(nrpts);
    for (int i = 0; i < nrpts; ++i)
    {
        testarray.push_back(Point{ 0,rvals(gen),thetavals(gen),phivals(gen) });
    }
    std::cout << "done.\n";
    std::cout << "First three points are: \n";
    for (int i = 0; i < 3; ++i)
    {
        std::cout << toString(testarray[i]) << "\n";
    }
    std::cout << "Will loop through these points " << nrloops << " times each iteration, for " << nrouterloops << " iterations.\n";

    TwoIndex tempMetricRet{};
    Timer theTimer;
    std::vector<TwoIndex> TestLastMetrics{};
    TestLastMetrics.reserve(nrtests);

    std::array<TestBase*, nrtests> TheTests{};
    std::array<int, nrtests> testorder{};
    TheTests[0] = new TestInheritance{};
    TheTests[1] = new TestStdVariantVisit{};
    TheTests[2] = new TestStdVariantVisitStruct{};
    TheTests[3] = new TestDirectFunctionCall{};
    TheTests[4] = new TestStdFunction{};
    //TheTests[5] = new TestStdGetLambda{};
   // TheTests[6] = new TestStdGetFunctionCall{};

    std::cout << "\n";
    for (int curloop = 0; curloop < nrouterloops; ++curloop)
    {
        std::cout << "Starting iteration " << curloop + 1 << "/" << nrouterloops << ":\n";

        for (int i = 0; i < nrtests; ++i)
            testorder[i] = i;
        std::shuffle(testorder.begin(), testorder.end(), gen);

        for (int i = 0; i < nrtests; ++i)
        {
            std::cout << "Calculating with method: " << TheTests[testorder[i]]->name() << "... ";
            TheTests[testorder[i]]->DoTest(theTimer, thea, testarray, TestLastMetrics);
            std::cout << " done in " << TheTests[testorder[i]]->LastTimeElapsed << "s (total accumulated: "
                << TheTests[testorder[i]]->TotalTimeElapsed << "s).\n";
        }


        std::cout << "All tests of this iteration done.\n";

        std::cout << "Testing to make sure last calculated element of all tests are all equal... ";
        for (int i = 0; i < nrtests; ++i)
        {
            for (int j = 0; j < dimension; ++j)
            {
                for (int k = 0; k < dimension; ++k)
                {
                    if (abs(TestLastMetrics[0][j][k] - TestLastMetrics[i][j][k]) > 1e-10)
                    {
                        std::cout << "Found inequality in 0 and " << i << ".\n";
                    }
                }
            }
        }
        std::cout << "done.\n";

        std::cout << "Test iteration " << curloop + 1 << "/" << nrouterloops << " done.\n\n";
    }

    std::cout << "All tests done!\n";

    std::array<double, nrtests> alltimes{};
    for (int i = 0; i < nrtests; i++)
        alltimes[i] = TheTests[i]->TotalTimeElapsed;

    double fastesttime{};
    std::cout << "Total accumulated times from fast to slow:\n";
    for (int i = 0; i < nrtests; ++i)
    {
        auto result = std::min_element(alltimes.begin(), alltimes.end());
        auto resplace= std::distance(alltimes.begin(), result);
        std::cout << "Method: " << TheTests[resplace]->name() << ": " << TheTests[resplace]->TotalTimeElapsed << "s";

        if (i == 0)
            fastesttime = TheTests[resplace]->TotalTimeElapsed;
        else
            std::cout << " (x " << TheTests[resplace]->TotalTimeElapsed / fastesttime << ")";
            
        std::cout << ".\n";
        alltimes[resplace] = 1000;
    }

}