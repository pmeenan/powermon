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
  std::string Measure();
  bool ok;

private:
  std::string GetCPUTime();
  std::string GetPowerData();
  std::string DoubleToString(double num);

  #ifdef WIN32
  ULARGE_INTEGER start_cpu_time;
  void GetWinCPUTime(ULARGE_INTEGER &cpu_time);
  #endif

  CIntelPowerGadgetLib intel;
};
