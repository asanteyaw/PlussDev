#pragma once

// Symbol visibility macros (shared library friendliness).
#if defined(_WIN32)
  #if defined(PLUSS_BUILD_SHARED)
    #define PLUSS_API __declspec(dllexport)
    #define MSHARP_API __declspec(dllexport)
  #elif defined(PLUSS_USE_SHARED)
    #define PLUSS_API __declspec(dllimport)
    #define MSHARP_API __declspec(dllimport)
  #else
    #define PLUSS_API
    #define MSHARP_API
  #endif
#else
  #if __GNUC__ >= 4
    #define PLUSS_API __attribute__((visibility("default")))
    #define MSHARP_API __attribute__((visibility("default")))
  #else
    #define PLUSS_API
    #define MSHARP_API
  #endif
#endif
