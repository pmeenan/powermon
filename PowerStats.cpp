#include "PowerStats.h"
#include <codecvt>

PowerStats::PowerStats() {
  #ifdef WIN32
  start_time.QuadPart = 0;
  start_cpu_time.QuadPart = 0;
  #endif

  intel.IntelEnergyLibInitialize();
}


PowerStats::~PowerStats() {
}


/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
std::string PowerStats::Start() {
  std::string response = "OK";
  #ifdef WIN32
  QueryPerformanceCounter(&start_time);
  GetWinCPUTime(start_cpu_time);
  #endif
  intel.ReadSample();
  return response;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
std::string PowerStats::Measure(const char * query_params) {
  std::string response("{\"type\":\"measurement\"");

  // Elapsed wall-clock (seconds)
  response += ",\"elapsed_time\":" + GetElapsedTime();

  // CPU busy time (seconds)
  response += ",\"cpu_time\":" + GetCPUTime();

  // Power Data
  response += GetPowerData();

  response += "}";
  return response;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
std::string PowerStats::GetElapsedTime() {
  double elapsed = 0;
  #ifdef WIN32
  if (start_time.QuadPart > 0) {
    LARGE_INTEGER now, freq;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&freq);
    elapsed = (double)(now.QuadPart - start_time.QuadPart) / (double)freq.QuadPart;
  }
  #endif
  return DoubleToString(elapsed);
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
std::string PowerStats::GetCPUTime() {
  double elapsed = 0;
  #ifdef WIN32
  if (start_cpu_time.QuadPart > 0) {
    ULARGE_INTEGER now;
    GetWinCPUTime(now);
    elapsed = (double)(now.QuadPart - start_cpu_time.QuadPart) / 10000000.0;
  }
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
std::string PowerStats::IntToString(int num) {
  std::string response = "";
  if (num) {
    char buff[100];
    sprintf(buff, "%d", num);
    response = buff;
  }
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

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
std::string PowerStats::GetPowerData() {
  std::string result = "";

  std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;

  intel.ReadSample();
  int nodes = 0;
  if (intel.GetNumNodes(&nodes) && nodes > 0) {
    int count = 0;
    if (intel.GetNumMsrs(&count)) {
      int funcID;
      wchar_t name[1024];
      std::string key;
      for (int msr = 0; msr < count; msr++) {
        intel.GetMsrName(msr, name);
        intel.GetMsrFunc(msr, &funcID);
        if (funcID == 1) {  // Power
          int nData = 0;
          double data[3];
          for (int node = 0; node < nodes; node++) {
            if (intel.GetPowerData(node, msr, data, &nData) && nData >= 3) {
              key = std::string(myconv.to_bytes(std::wstring(name)));
              result += ",\"" + key + IntToString(node) + " Joules\":" + DoubleToString(data[1]);
              result += ",\"" + key + IntToString(node) + " mWh\":" + DoubleToString(data[2]);
            }
          }
        }
      }
    }
  }
  return result;
}

#endif // WIN32