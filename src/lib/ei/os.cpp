#include "EdAsmNg/ei/os.hpp"

#include <limits.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <string>

#include "EdAsmNg/ei/screen.hpp"

namespace fs = std::filesystem;

std::uint16_t P8DATE = 0;
std::uint16_t P8TIME = 0;

std::uint16_t PackProDOSDate(const std::tm& tm) {
  const auto year  = static_cast<std::uint16_t>((tm.tm_year + 1900) - 1900);
  const auto month = static_cast<std::uint16_t>(tm.tm_mon + 1);
  const auto day   = static_cast<std::uint16_t>(tm.tm_mday);
  return static_cast<std::uint16_t>((year << 9) | (month << 5) | day);
}

namespace {
  std::uint16_t PackProDOSTime(const std::tm& tm) {
    const auto hour   = static_cast<std::uint16_t>(tm.tm_hour);
    const auto minute = static_cast<std::uint16_t>(tm.tm_min);
    const auto second = static_cast<std::uint16_t>(tm.tm_sec / 2);  // 2-second granularity
    return static_cast<std::uint16_t>((hour << 11) | (minute << 5) | second);
  }
}  // namespace

void GETDATETIME() {
  using clock           = std::chrono::system_clock;
  const std::time_t now = clock::to_time_t(clock::now());

  std::tm local{};
#if defined(_WIN32)
  localtime_s(&local, &now);
#else
  localtime_r(&now, &local);
#endif

  P8DATE = PackProDOSDate(local);
  P8TIME = PackProDOSTime(local);
}

// ProDOS MLI $C7: Get current prefix → Linux getcwd
std::string GetProDOSPrefix() {
  std::error_code ec;
  fs::path        cwd = fs::current_path(ec);
  return ec ? "" : cwd.string();
}

// ProDOS MLI $C6: Set prefix → Linux chdir
bool SetProDOSPrefix(const std::string& path) {
  return chdir(path.c_str()) == 0;
}

// ProDOS MLI $C5: Get online volume → Linux root or mount point
std::string GetOnlineVolume(std::uint8_t /* unit */) {
  // ProDOS volumes map to Linux root or specific mount points
  // For simplicity, just return root
  return "/";  // or parse /proc/mounts if you need actual volumes
}

std::string ExecutablePath() {
  char    buf[PATH_MAX] = {};
  ssize_t len           = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  return (len > 0) ? std::string(buf, static_cast<size_t>(len)) : std::string();
}

void PRERR() {
  COUT('E');
  COUT('R');
  COUT('R');
  COUT(BEL);
}

void FDD3(int A) {
  const char hex[] = "0123456789ABCDEF";
  COUT('=');
  char buf[sizeof(int) * 2 + 1] = {};
  snprintf(buf, sizeof(buf), "%02X", A);
  for (char c : buf) {
    COUT(c);
  }
}

void MON() {
  // In a real implementation, this would jump to the monitor.
  // For this example, we'll just print a message and exit.
  COUT(BEL);
  COUT('\r');
  COUT('*');
  sleep(10);
  COUT('\r');
  exit(EXIT_FAILURE);
}