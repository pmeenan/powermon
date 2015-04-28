#include "PowerStats.h"
#include <codecvt>

PowerStats::PowerStats() {
  #ifdef WIN32
  start_cpu_time.QuadPart = 0;
  #endif

  ok = intel.IntelEnergyLibInitialize();
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

  // CPU busy time (seconds)
  response += "\"cpu_time\":" + GetCPUTime();

  // Power Data (including elapsed time)
  response += GetPowerData();

  response += "}";
  return response;
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

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
std::string PowerStats::GetPowerData() {
  std::string result = "";

  std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;

  intel.ReadSample();
  double elapsed = 0;
  if (intel.GetTimeInterval(&elapsed))
    result += ",\"elapsed_time\":" + DoubleToString(elapsed);

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
          double joules = 0;
          double mWh = 0;
          for (int node = 0; node < nodes; node++) {
            if (intel.GetPowerData(node, msr, data, &nData) && nData >= 3) {
              joules += data[1];
              mWh += data[2];
            }
          }
          key = std::string(myconv.to_bytes(std::wstring(name)));
          result += ",\"" + key + " Joules\":" + DoubleToString(data[1]);
          result += ",\"" + key + " mWh\":" + DoubleToString(data[2]);
        }
      }
    }
  }
  return result;
}

#endif // WIN32