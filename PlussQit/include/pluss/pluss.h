#pragma once

// Primary user-facing umbrella include.
//
// For CodeM (Series/Tensor-style API), this exports:
// - pluss::Series
// - pluss::value(...)
// - pluss::make_series(...)
// - pluss::zeros/ones/full/rand/... factories
// - pluss::from_torch(...) / pluss::to_torch(...) (when PLUSS_ENABLE_TORCH=ON)
//
// For CodeN (text API), this exports:
// - pluss::Note
// - pluss::TokenBatch
// - pluss::content(...) / pluss::make_note(...)
//
// For CodeH (time API), this exports:
// - pluss::codeh::Timeline / Duration / TimeIndex
// - pluss::moment(...) / make_timeline(...) / from_epoch_nanos(...)
// - pluss::duration_* helpers / make_timeindex(...)
//
// Keep this include thin by delegating to module-level umbrellas.
#include "pluss/codem/series_factory.h"
#include "pluss/coden/note_factory.h"
#include "pluss/codeh/timeline_factory.h"
#include "pluss/codeh/timeindex_factory.h"
#include "aux/UseAux.h"
