#pragma once

// Minimal deprecated macro stub for compatibility with upstream headers
#ifndef C10_DEPRECATED
#if defined(__GNUC__) || defined(__clang__)
#define C10_DEPRECATED(msg) __attribute__((deprecated(msg)))
#else
#define C10_DEPRECATED(msg)
#endif
#endif

#ifndef C10_DEPRECATED_MESSAGE
#define C10_DEPRECATED_MESSAGE(msg) C10_DEPRECATED(msg)
#endif
