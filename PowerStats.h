#ifdef WIN32
#include <Windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <string>
#ifdef WIN32
#include "lib/intel/IntelPowerGadgetLib.h"
#endif

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
  std::string GetElapsedTime();
  std::string DoubleToString(double num);

  #ifdef WIN32
  ULARGE_INTEGER start_cpu_time;
  void GetWinCPUTime(ULARGE_INTEGER &cpu_time);
  CIntelPowerGadgetLib intel;
  #else
  uint64_t start_time;
  uint64_t start_cpu_time;
  uint64_t GetMacCPUTime();
  #endif
    
  // Wrappers around the Intel API's
  bool Intel_ReadSample();
  bool Intel_GetTimeInterval(double &interval);
  bool Intel_GetNumNodes(int &count);
  bool Intel_GetNumMsrs(int &count);
  bool Intel_GetMsrName(int iMsr, std::string &name);
  bool Intel_GetMsrFunc(int iMsr, int &funcID);
  bool Intel_GetPowerData(int iNode, int iMSR, double *pResult, int *nResult);
};
