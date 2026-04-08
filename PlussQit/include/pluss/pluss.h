#pragma once

// Primary user-facing umbrella include.
//
// For CodeM (Series/Tensor-style API), this exports:
// - pluss::Series
// - pluss::make_series(...)
// - pluss::from_buffer(...)
// - pluss::zeros/ones/full/rand/... factories
// - pluss::distr::{sample,sample_like,pdf,logpdf,cdf,mixture_normal} when PlussMa is available
// - pluss::from_torch(...) / pluss::to_torch(...) (when PLUSS_ENABLE_TORCH=ON)
//
// For CodeN (text API), this exports:
// - pluss::Note
// - pluss::TokenBatch
// - pluss::make_note(...) / pluss::make_note(...)
//
// For CodeH (time API), this exports:
// - pluss::codeh::Timeline / Duration / TimeIndex
// - pluss::make_timeline(...) / make_timeline(...) / from_epoch_nanos(...)
// - pluss::duration_* helpers / make_timeindex(...)
//
// Unified serialization (core types + supported STL containers):
// - pluss::io::save(...) / pluss::io::load<...>(...)
// - pluss::save(...) / pluss::load<...>(...) aliases
//
// Validation facade (function-style checks):
// - pluss::validate::check(...) / warn(...) / fail(...) / inspect(...) / check_eq(...)
//
// Dynamic union holder:
// - pluss::Type (Series/Note/Timeline/TimeIndex)
// - pluss::Type::invoke("method", args...) for dynamic method-name dispatch on Type
// - pluss::dispense<R>(obj, "method", args...) typed dynamic dispatch
// - pluss::dispense_any(obj, "method", args...) runtime variant dispatch
//
// Core pluss surface (Series/Note/Timeline/TimeIndex + Type/Dispense) lives in PlussQit.
#include "pluss/core.h"

// PlussAI is intentionally opt-in:
// - include <pluss/nn_func.h> for lightweight functional neural surfaces
// - include <pluss/nn.h> for full neural module stack
// - include <pluss/ai.h> for neural + classical ML (NodeJ) surface
//
// PlussMa is intentionally opt-in:
// - include <pluss/ma.h> for PlussMa math/stat/time-series utilities
//
// PlussTils is intentionally opt-in:
// - include <pluss/tils.h> for utility helpers (aux + view utilities), including:
//   - pluss::transmit(...), pluss::transform_axis(...)
//   - pluss::DataLoader + pluss::loader::* helpers
//   - pluss::to_scalar<T>(series) + pluss::as_f64/as_f32/as_i64/... helpers
//   - pluss::view::* utilities

#if __has_include("pluss/mic.h")
#include "pluss/mic.h"
#endif

#if __has_include("pluss/pl.h")
#include "pluss/pl.h"
#endif
