#pragma once

// Core pluss surface owned by PlussQit.
// This header intentionally excludes higher-level libraries (Mic/Ma/Pl).

#include "pluss/codem/series_factory.h"
#include "pluss/codem/losses.h"
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

#if __has_include("subsp/stats/stats.h")
#include "subsp/stats/stats.h"
#elif __has_include("pluss/subsp/stats/stats.h")
#include "pluss/subsp/stats/stats.h"
#endif

#include "pluss/queue.h"
#include "pluss/dispense.h"
#include "aux/UseAux.h"
