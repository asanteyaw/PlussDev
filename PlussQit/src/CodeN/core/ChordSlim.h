#pragma once

// ChordSlim: exposes the Chord handle type without pulling the optional heavy surface.
// Preferred include in low-level kernels and headers.

#ifndef CODEN_NO_CHORD_SURFACE
#define CODEN_NO_CHORD_SURFACE 1
#define CODEN_SLIM_DEFINED_NO_CHORD_SURFACE 1
#endif

#ifndef CODEN_ASSERT_NO_CHORD_SURFACE
#define CODEN_ASSERT_NO_CHORD_SURFACE 1
#define CODEN_SLIM_DEFINED_ASSERT_NO_CHORD_SURFACE 1
#endif

#include "CodeN/core/Chord.h"

#if defined(CODEN_SLIM_DEFINED_NO_CHORD_SURFACE)
#undef CODEN_NO_CHORD_SURFACE
#undef CODEN_SLIM_DEFINED_NO_CHORD_SURFACE
#endif

#if defined(CODEN_SLIM_DEFINED_ASSERT_NO_CHORD_SURFACE)
#undef CODEN_ASSERT_NO_CHORD_SURFACE
#undef CODEN_SLIM_DEFINED_ASSERT_NO_CHORD_SURFACE
#endif
