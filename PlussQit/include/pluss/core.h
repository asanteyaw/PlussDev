#pragma once

// Core pluss surface owned by PlussQit.
// This header intentionally excludes higher-level libraries (Mic/Ma/Pl).

#include "pluss/codem/series_factory.h"
#include "pluss/coden/note_factory.h"
#include "pluss/codeh/timeline_factory.h"
#include "pluss/codeh/timeindex_factory.h"

#if __has_include("subsp/math/linalg.h")
#include "subsp/math/linalg.h"
#elif __has_include("pluss/subsp/math/linalg.h")
#include "pluss/subsp/math/linalg.h"
#endif

#if __has_include("subsp/linearalg/linearalg.h")
#include "subsp/linearalg/linearalg.h"
#elif __has_include("pluss/subsp/linearalg/linearalg.h")
#include "pluss/subsp/linearalg/linearalg.h"
#endif

#if __has_include("subsp/math/diff.h")
#include "subsp/math/diff.h"
#elif __has_include("pluss/subsp/math/diff.h")
#include "pluss/subsp/math/diff.h"
#endif

#if __has_include("subsp/math/optim.h")
#include "subsp/math/optim.h"
#elif __has_include("pluss/subsp/math/optim.h")
#include "pluss/subsp/math/optim.h"
#endif

#if __has_include("subsp/stat/stats.h")
#include "subsp/stat/stats.h"
#elif __has_include("pluss/subsp/stat/stats.h")
#include "pluss/subsp/stat/stats.h"
#endif

#include "pluss/queue.h"
#include "pluss/dispense.h"
#include "aux/UseAux.h"
