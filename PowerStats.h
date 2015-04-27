#ifdef WIN32
#include <Windows.h>
#endif WIN32
#include <stdio.h>
#include <string.h>
#include <string>
#include "lib/intel/IntelPowerGadgetLib.h"

class PowerStats {
public:
  PowerStats();
  ~PowerStats();
  std::string Start();
  std::string Measure(const char * query_params);

private:
  std::string GetElapsedTime();
  std::string GetCPUTime();
  std::string GetPowerData();
  std::string DoubleToString(double num);
  std::string IntToString(int num);

  #ifdef WIN32
  LARGE_INTEGER  start_time;
  ULARGE_INTEGER start_cpu_time;
  void GetWinCPUTime(ULARGE_INTEGER &cpu_time);
  #endif

  CIntelPowerGadgetLib intel;
};
