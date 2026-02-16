#pragma once

#include <cstdint>
#include <ctime>
#include <string>

// ProDOS-style date/time words (P8DATE/P8TIME) stored in host memory.
extern std::uint16_t P8DATE;
extern std::uint16_t P8TIME;
// Last device accessed (slot << 4 | drive)
const int LASTDEV = 0;

// Populate P8DATE/P8TIME with the current local date/time in ProDOS packed format.
void GETDATETIME();

// Pack a tm structure into ProDOS 16-bit date format.
// Bits 9-15: year (0-127, years since 1900)
// Bits 5-8: month (1-12)
// Bits 0-4: day (1-31)
std::uint16_t PackProDOSDate(const std::tm& tm);

// ProDOS MLI $C7: Get current prefix (working directory)
std::string GetProDOSPrefix();

// ProDOS MLI $C6: Set prefix (change working directory)
bool SetProDOSPrefix(const std::string& path);

// ProDOS MLI $C5: Get online volume name for a device unit
std::string GetOnlineVolume(std::uint8_t unit);

std::string ExecutablePath();

// Print "ERR" + BEL (matches original monitor error prefix).
void PRERR();

// Print hex byte after "=" (monitor entry point $FDD3 behavior).
void FDD3(int A);

// Jump to monitor (monitor entry point $FF65 behavior).
void MON();