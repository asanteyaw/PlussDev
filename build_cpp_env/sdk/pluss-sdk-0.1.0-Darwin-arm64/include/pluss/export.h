#pragma once

// Export / import macros (Torch-inspired, modernized).
//
// The build system should define:
//   - PLUSS_BUILD_SHARED_LIBS when building shared libraries
//   - MSHARP_BUILD_MAIN_LIB for the MSharp shared library itself
//   - CODEM_BUILD_MAIN_LIB  for the CodeM shared library itself
//
// Consumers should NOT define *_BUILD_MAIN_LIB; they will see IMPORT.

#ifdef _WIN32
  #define PLUSS_HIDDEN
  #if defined(PLUSS_BUILD_SHARED_LIBS)
    #define PLUSS_EXPORT __declspec(dllexport)
    #define PLUSS_IMPORT __declspec(dllimport)
  #else
    #define PLUSS_EXPORT
    #define PLUSS_IMPORT
  #endif
#else
  #if defined(__GNUC__)
    #define PLUSS_EXPORT __attribute__((__visibility__("default")))
    #define PLUSS_HIDDEN __attribute__((__visibility__("hidden")))
  #else
    #define PLUSS_EXPORT
    #define PLUSS_HIDDEN
  #endif
  #define PLUSS_IMPORT PLUSS_EXPORT
#endif

// MSharp (Torch's c10)
#ifdef MSHARP_BUILD_MAIN_LIB
  #define MSHARP_API PLUSS_EXPORT
#else
  #define MSHARP_API PLUSS_IMPORT
#endif

// Flame (CodeN's foundation layer; alias of MSharp for now)
#ifndef FLAME_API
  #define FLAME_API MSHARP_API
#endif

// CodeM (Torch's ATen/torch)
#ifdef CODEM_BUILD_MAIN_LIB
  #define CODEM_API PLUSS_EXPORT
#else
  #define CODEM_API PLUSS_IMPORT
#endif


// CodeN (text: Note)
#ifdef CODEN_BUILD_MAIN_LIB
  #define CODEN_API PLUSS_EXPORT
#else
  #define CODEN_API PLUSS_IMPORT
#endif

// CodeH (time: Timeline/Duration and Timeline's index accessor)
#ifdef CODEH_BUILD_MAIN_LIB
  #define CODEH_API PLUSS_EXPORT
#else
  #define CODEH_API PLUSS_IMPORT
#endif

// Backward compatibility:
// Existing code may use PLUSS_API as the main public macro.
#ifndef PLUSS_API
  #define PLUSS_API CODEM_API
#endif
