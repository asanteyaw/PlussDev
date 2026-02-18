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
// Keep this include thin by delegating to module-level umbrellas.
#include "pluss/codem/series_factory.h"
#include "pluss/coden/note_factory.h"
