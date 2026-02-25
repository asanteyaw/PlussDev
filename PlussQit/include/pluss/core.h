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

#if __has_include("subsp/math/optim.h")
#include "subsp/math/optim.h"
#elif __has_include("pluss/subsp/math/optim.h")
#include "pluss/subsp/math/optim.h"
#endif

#include "pluss/queue.h"
#include "pluss/dispense.h"
#include "aux/UseAux.h"
