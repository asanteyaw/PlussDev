#pragma once

// "Slim" Series include: provides the Series handle type and core helpers but
// intentionally *omits* the generated op surface.
//
// This is useful for core headers and lower-level code where you want to avoid
// pulling in thousands of wrappers (and to prevent circular includes).

#define CODEM_NO_SERIES_SURFACE 1
#define CODEM_ASSERT_NO_SERIES_SURFACE 1
#include "CodeM/core/SeriesSlim.h"
#undef CODEM_ASSERT_NO_SERIES_SURFACE
#undef CODEM_NO_SERIES_SURFACE
