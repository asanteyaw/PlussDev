#pragma once

// Public API macro (TORCH_API analog)
#if defined(_WIN32)
  #if defined(PLUSS_BUILD_SHARED)
    #define PLUSS_API __declspec(dllexport)
  #else
    #define PLUSS_API __declspec(dllimport)
  #endif
#else
  #define PLUSS_API __attribute__((visibility("default")))
#endif
