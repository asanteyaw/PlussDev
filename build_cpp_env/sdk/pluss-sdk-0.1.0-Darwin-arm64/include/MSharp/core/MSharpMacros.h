#pragma once

// MSharp is the replacement for c10.
// This header is intentionally small in v0; expand as CodeM grows.

#if defined(_WIN32)
  #if defined(PLUSSQIT_BUILDING_LIBRARY)
    #define PLUSS_API __declspec(dllexport)
  #else
    #define PLUSS_API __declspec(dllimport)
  #endif
#else
  #define PLUSS_API __attribute__((visibility("default")))
#endif
