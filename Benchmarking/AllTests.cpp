#include"AllTests.h"

void DummyTest(const TwoIndex& dummyTwo, const Point& dummyP)
{
    if (dummyTwo[0][0] == dummyP[1] || dummyTwo[2][2] == dummyP[0])
        std::cout << "I am a dummy; here to prevent the compiler from entirely compiling me away.";
}

std::string TestInheritance::name() { return "inheritance"; }
std::string TestStdVariantVisit::name() { return "std::variant/visit"; }
std::string TestStdVariantVisitStruct::name() { return "std::variant/visit with pre-initialized struct"; }
std::string TestDirectFunctionCall::name() { return "direct function call"; }
std::string TestStdFunction::name() { return "std::function"; }
std::string TestStdGetLambda::name() { return "std::get lambda function"; }
std::string TestStdGetFunctionCall::name() { return "std::get function call"; }

void TestInheritance::DoTest(Timer& theTimer, real thea, std::vector<Point> testarray, std::vector<TwoIndex>TestLastMetrics)
{
    // initialization
    //std::unique_ptr<KerrMetricINHERITANCE> m{ new KerrMetricINHERITANCE{thea} };
    //KerrMetricINHERITANCE const* m = new KerrMetricINHERITANCE{ thea };
    //KerrMetricINHERITANCE m{ thea };
    std::unique_ptr<Metric> m{ new KerrMetricINHERITANCE{thea} };
    TwoIndex tempMetricRet{};

    // loop
    theTimer.reset();
    for (int j = 0; j < nrloops; ++j)
    {
        for (const Point& r : testarray)
        {
            tempMetricRet = m->getMetric_dd(r);
            //tempMetricRet = m.getMetric_dd(r);
            DummyTest(tempMetricRet, r);
        }
    }
    LastTimeElapsed = theTimer.elapsed();
    TotalTimeElapsed += LastTimeElapsed;
    TestLastMetrics.push_back(tempMetricRet);


    // clean up
    //delete m;
    //m = nullptr;
}

 void TestStdVariantVisit::DoTest(Timer& theTimer, real thea, std::vector<Point> testarray, std::vector<TwoIndex>TestLastMetrics)
 {
     // initialization
     MetricVariantObj VarMetric{ KerrMetricVARIANT{thea} };
     TwoIndex tempMetricRet{};

     // loop
     theTimer.reset();
     for (int j = 0; j < nrloops; ++j)
     {
         for (const Point& r : testarray)
         {
             tempMetricRet = std::visit(getMetric_dd_VISITOR{ r }, VarMetric);
             DummyTest(tempMetricRet, r);
         }
     }
     LastTimeElapsed = theTimer.elapsed();
     TotalTimeElapsed += LastTimeElapsed;
     TestLastMetrics.push_back(tempMetricRet);
 }

 void TestStdVariantVisitStruct::DoTest(Timer& theTimer, real thea, std::vector<Point> testarray, std::vector<TwoIndex>TestLastMetrics)
 {
     // initialization
     MetricVariantObj VarMetric{ KerrMetricVARIANT{thea} };
     getMetric_dd_VISITOR theVISstruct{ };
     TwoIndex tempMetricRet{};

     // loop
     theTimer.reset();
     for (int j = 0; j < nrloops; ++j)
     {
         for (const Point& r : testarray)
         {
             theVISstruct.p = r;
             tempMetricRet = std::visit(theVISstruct, VarMetric);
             DummyTest(tempMetricRet, r);
         }
     }
     LastTimeElapsed = theTimer.elapsed();
     TotalTimeElapsed += LastTimeElapsed;
     TestLastMetrics.push_back(tempMetricRet);
 }

 void TestDirectFunctionCall::DoTest(Timer& theTimer, real thea, std::vector<Point> testarray, std::vector<TwoIndex>TestLastMetrics)
 {
     // initialization
     TwoIndex tempMetricRet{};

     // loop
     theTimer.reset();
     for (int j = 0; j < nrloops; ++j)
     {
         for (const Point& r : testarray)
         {
             tempMetricRet = MetricDIRECTCALL(r,thea);
             DummyTest(tempMetricRet, r);
         }
     }
     LastTimeElapsed = theTimer.elapsed();
     TotalTimeElapsed += LastTimeElapsed;
     TestLastMetrics.push_back(tempMetricRet);
 }

 void TestStdFunction::DoTest(Timer& theTimer, real thea, std::vector<Point> testarray, std::vector<TwoIndex>TestLastMetrics)
 {
     // initialization
     std::function<TwoIndex(Point)> TheMetricFunc;
     MetricVariantObj VarMetric{ KerrMetricVARIANT{thea} };
     TheMetricFunc = std::visit(getCorrectMetricFunctionVARIANT{}, VarMetric);
     TwoIndex tempMetricRet{};

     // loop
     theTimer.reset();
     for (int j = 0; j < nrloops; ++j)
     {
         for (const Point& r : testarray)
         {
             tempMetricRet = TheMetricFunc(r);
             DummyTest(tempMetricRet, r);
         }
     }
     LastTimeElapsed = theTimer.elapsed();
     TotalTimeElapsed += LastTimeElapsed;
     TestLastMetrics.push_back(tempMetricRet);
 }

 void TestStdGetLambda::DoTest(Timer& theTimer, real thea, std::vector<Point> testarray, std::vector<TwoIndex>TestLastMetrics)
 {
     // initialization
     MetricVariantObj VarMetric{ KerrMetricVARIANT{thea} };

     auto TheLambdaFunc = [VarMetric](Point r) { return getMetric_dd_IMPROVED_VARIANT{}(std::get<0>(VarMetric), r); };
     TwoIndex tempMetricRet{};

     // loop
     theTimer.reset();
     for (int j = 0; j < nrloops; ++j)
     {
         for (const Point& r : testarray)
         {
             tempMetricRet = TheLambdaFunc(r);
             DummyTest(tempMetricRet, r);
         }
     }
     LastTimeElapsed = theTimer.elapsed();
     TotalTimeElapsed += LastTimeElapsed;
     TestLastMetrics.push_back(tempMetricRet);
 }

 void TestStdGetFunctionCall::DoTest(Timer& theTimer, real thea, std::vector<Point> testarray, std::vector<TwoIndex>TestLastMetrics)
 {
     MetricVariantObj VarMetric{ KerrMetricVARIANT{thea} };
     TwoIndex tempMetricRet{};

     // loop
     theTimer.reset();
     for (int j = 0; j < nrloops; ++j)
     {
         for (const Point& r : testarray)
         {
             tempMetricRet = getMetricSTDGET(VarMetric, r);
             DummyTest(tempMetricRet, r);
         }
     }
     LastTimeElapsed = theTimer.elapsed();
     TotalTimeElapsed += LastTimeElapsed;
     TestLastMetrics.push_back(tempMetricRet);
 }