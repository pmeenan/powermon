#include "PowerStats.h"
#include <codecvt>
#ifndef WIN32
#include <IntelPowerGadget/EnergyLib.h>
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <mach/mach_host.h>
#include <mach/mach_time.h>
#endif

PowerStats::PowerStats() {
  #ifdef WIN32
  start_cpu_time.QuadPart = 0;
  ok = intel.IntelEnergyLibInitialize();
  #else
  start_time = 0;
  start_cpu_time = 0;
  ok = IntelEnergyLibInitialize() != 0;
  #endif
}


PowerStats::~PowerStats() {
}


/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
std::string PowerStats::Start() {
  // Just trigger a throw-away measurement
  Measure();
  return "OK";
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
std::string PowerStats::Measure() {
  std::string response("{");

  response += "\"elapsed_time\":" + GetElapsedTime();

  // CPU busy time (seconds)
  response += ",\"cpu_time\":" + GetCPUTime();

  // Power Data (including elapsed time)
  response += GetPowerData();

  response += "}";
  return response;
}

/*-----------------------------------------------------------------------------
 -----------------------------------------------------------------------------*/
std::string PowerStats::GetElapsedTime() {
  double elapsed = 0;
#ifdef WIN32
#else
  uint64_t now = mach_absolute_time();
  if (start_time > 0) {
    mach_timebase_info_data_t info;
    if (mach_timebase_info(&info) == KERN_SUCCESS) {
      uint64_t nanos = ((now - start_time) * info.numer) / info.denom;
      elapsed = (double)nanos / NSEC_PER_SEC;
    }
  }
  start_time = now;
#endif
  return DoubleToString(elapsed);
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
std::string PowerStats::GetCPUTime() {
  double elapsed = 0;
#ifdef WIN32
  ULARGE_INTEGER now;
  GetWinCPUTime(now);
  if (start_cpu_time.QuadPart > 0)
    elapsed = (double)(now.QuadPart - start_cpu_time.QuadPart) / 10000000.0;
  start_cpu_time.QuadPart = now.QuadPart;
#else
  uint64_t now = GetMacCPUTime();
  if (start_cpu_time > 0)
    elapsed = (double)(now - start_cpu_time) / 100.0;
  start_cpu_time = now;
#endif
  return DoubleToString(elapsed);
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
std::string PowerStats::DoubleToString(double num) {
  std::string response = "";
  char buff[100];
  sprintf(buff, "%0.6f", num);
  response = buff;
  return response;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
#ifdef WIN32
void PowerStats::GetWinCPUTime(ULARGE_INTEGER &cpu_time) {
  FILETIME idle_time, kernel_time, user_time;
  if (GetSystemTimes(&idle_time, &kernel_time, &user_time)) {
    ULARGE_INTEGER k, u, i, total;
    k.LowPart = kernel_time.dwLowDateTime;
    k.HighPart = kernel_time.dwHighDateTime;
    u.LowPart = user_time.dwLowDateTime;
    u.HighPart = user_time.dwHighDateTime;
    i.LowPart = idle_time.dwLowDateTime;
    i.HighPart = idle_time.dwHighDateTime;
    total.QuadPart = (k.QuadPart + u.QuadPart);
    cpu_time.QuadPart = total.QuadPart - i.QuadPart;
  }
}
#else
uint64_t PowerStats::GetMacCPUTime() {
  uint64_t cpu_time = 0;
  processor_cpu_load_info_t cpuLoad;
  mach_msg_type_number_t processorMsgCount;
  natural_t processorCount;
  
  host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &processorCount, (processor_info_array_t *)&cpuLoad, &processorMsgCount);
  for (natural_t i = 0; i < processorCount; i++) {
    cpu_time += cpuLoad[i].cpu_ticks[CPU_STATE_SYSTEM];
    cpu_time += cpuLoad[i].cpu_ticks[CPU_STATE_USER];
    cpu_time += cpuLoad[i].cpu_ticks[CPU_STATE_NICE];
  }
  return cpu_time;
}
#endif // WIN32

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
std::string PowerStats::GetPowerData() {
  std::string result = "";

  Intel_ReadSample();
  int nodes = 0;
  if (Intel_GetNumNodes(nodes) && nodes > 0) {
    int count = 0;
    if (Intel_GetNumMsrs(count)) {
      int funcID;
      std::string name;
      for (int msr = 0; msr < count; msr++) {
        Intel_GetMsrName(msr, name);
        Intel_GetMsrFunc(msr, funcID);
        if (funcID == 1) {  // Power
          int nData = 0;
          double data[3];
          double joules = 0;
          double mWh = 0;
          for (int node = 0; node < nodes; node++) {
            if (Intel_GetPowerData(node, msr, data, &nData) && nData >= 3) {
              joules += data[1];
              mWh += data[2];
            }
          }
          result += ",\"" + name + " Joules\":" + DoubleToString(data[1]);
          result += ",\"" + name + " mWh\":" + DoubleToString(data[2]);
        }
      }
    }
  }
  return result;
}

/*-----------------------------------------------------------------------------
  Windows implementations of the Intel API wrappers
 -----------------------------------------------------------------------------*/
#ifdef WIN32
bool PowerStats::Intel_ReadSample() {
  return intel.ReadSample();
}

bool PowerStats::Intel_GetTimeInterval(double &interval){
  return intel.GetTimeInterval(&interval);
}

bool PowerStats::Intel_GetNumNodes(int &count){
  return intel.GetNumNodes(&count);
}

bool PowerStats::Intel_GetNumMsrs(int &count){
  return intel.GetNumMsrs(&count);
}

bool PowerStats::Intel_GetMsrName(int iMsr, std::string &name){
  wchar_t buff[1024];
  bool ok = intel.GetMsrName(iMsr, buff);
  if (ok)
    name = std::string(myconv.to_bytes(std::wstring(buff)));
  return ok;
}

bool PowerStats::Intel_GetMsrFunc(int iMsr, int &funcID){
  return intel.GetMsrFunc(iMsr, &funcID)
}

bool PowerStats::Intel_GetPowerData(int iNode, int iMSR, double *pResult, int *nResult){
  return intel.GetPowerData(iNode, iMSR, pResult, nResult);
}
#else //WIN32
/*-----------------------------------------------------------------------------
 Mac implementations of the Intel API wrappers
 -----------------------------------------------------------------------------*/
bool PowerStats::Intel_ReadSample() {
  return ReadSample() != 0;
}

bool PowerStats::Intel_GetTimeInterval(double &interval){
  return GetTimeInterval(&interval) != 0;
}

bool PowerStats::Intel_GetNumNodes(int &count){
  return GetNumNodes(&count) != 0;
}

bool PowerStats::Intel_GetNumMsrs(int &count){
  return GetNumMsrs(&count) != 0;
}

bool PowerStats::Intel_GetMsrName(int iMsr, std::string &name){
  bool ok = false;
  char buff[1024];
  if (GetMsrName(iMsr, buff)) {
    ok = true;
    name = buff;
  }
  return ok;
}

bool PowerStats::Intel_GetMsrFunc(int iMsr, int &funcID){
  return GetMsrFunc(iMsr, &funcID) != 0;
}

bool PowerStats::Intel_GetPowerData(int iNode, int iMSR, double *pResult, int *nResult){
  return ::GetPowerData(iNode, iMSR, pResult, nResult) != 0;
}
#endif //WIN32