#pragma once

#if defined(_WIN32)
#include <MSharp/util/Exception.h>
#include <MSharp/util/win32-headers.h>
#include <string>
#endif

namespace MSharp {
#if defined(_WIN32)
C10_API std::wstring u8u16(const std::string& str);
C10_API std::string u16u8(const std::wstring& wstr);
#endif
} // namespace MSharp
