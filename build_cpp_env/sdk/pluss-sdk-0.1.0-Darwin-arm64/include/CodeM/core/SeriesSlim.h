#pragma once

// "Slim" Series include: provides the Series handle type and core helpers but
// intentionally *omits* the generated op surface.
//
// This header intentionally includes the main `Series.h` while defining
// `CODEM_NO_SERIES_SURFACE` so that the heavy generated surface (wrappers)
// is not pulled into translation units that only need the handle/type defs.

#define CODEM_NO_SERIES_SURFACE 1
#define CODEM_ASSERT_NO_SERIES_SURFACE 1
#include "CodeM/core/Series.h"
#undef CODEM_ASSERT_NO_SERIES_SURFACE
#undef CODEM_NO_SERIES_SURFACE
