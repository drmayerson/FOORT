#ifndef _FOORT_ALLTESTS_BENCH_H
#define _FOORT_ALLTESTS_BENCH_H

#include <iostream>
#include<chrono>

#include "Geometry.h"
#include"Metrics_all.h"

#include<variant>
#include<random>
#include<array>
#include<string>




constexpr int nrpts = 1000;
constexpr int nrloops = 100000;

class Timer
{
private:
    // Type aliases to make accessing nested type easier
    //using Clock = std::chrono::steady_clock;
    using Clock = std::chrono::high_resolution_clock;
    using Second = std::chrono::duration<double, std::ratio<1> >;

    std::chrono::time_point<Clock> m_beg{ Clock::now() };

public:
    void reset()
    {
        m_beg = Clock::now();
    }

    double elapsed() const
    {
        return std::chrono::duration_cast<Second>(Clock::now() - m_beg).count();
    }
};

void DummyTest(const TwoIndex& dummyTwo, const Point& dummyP);

struct TestBase
{
    double TotalTimeElapsed{};
    double LastTimeElapsed{};

    virtual std::string name() = 0;
    virtual void DoTest(Timer& theTimer, real thea, std::vector<Point> testarray, std::vector<TwoIndex>TestLastMetrics) = 0;
};

struct TestInheritance : public TestBase
{
    std::string name() final;
     void DoTest(Timer& theTimer, real thea, std::vector<Point> testarray, std::vector<TwoIndex>TestLastMetrics) final;
};

struct TestStdVariantVisit : public TestBase
{
    std::string name() final;
    void DoTest(Timer& theTimer, real thea, std::vector<Point> testarray, std::vector<TwoIndex>TestLastMetrics) final;
};

struct TestStdVariantVisitStruct : public TestBase
{
    std::string name() final;
    void DoTest(Timer& theTimer, real thea, std::vector<Point> testarray, std::vector<TwoIndex>TestLastMetrics) final;
};

struct TestDirectFunctionCall : public TestBase
{
    std::string name() final;
    void DoTest(Timer& theTimer, real thea, std::vector<Point> testarray, std::vector<TwoIndex>TestLastMetrics) final;
};

struct TestStdFunction : public TestBase
{
    std::string name() final;
    void DoTest(Timer& theTimer, real thea, std::vector<Point> testarray,  std::vector<TwoIndex>TestLastMetrics) final;
};

struct TestStdGetLambda : public TestBase
{
    std::string name() final;
    void DoTest(Timer& theTimer, real thea, std::vector<Point> testarray,  std::vector<TwoIndex>TestLastMetrics) final;
};

struct TestStdGetFunctionCall : public TestBase
{
    std::string name() final;
    void DoTest(Timer& theTimer, real thea, std::vector<Point> testarray, std::vector<TwoIndex>TestLastMetrics) final;
};



#endif
