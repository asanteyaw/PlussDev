#pragma once

// Primary user-facing umbrella include.
//
// For CodeM (Series/Tensor-style API), this exports:
// - pluss::Series
// - pluss::make_series(...)
// - pluss::series::from_host(...)
// - pluss::zeros/ones/full/rand/... factories
// - pluss::series::print(...)
// - pluss::distr::{sample,sample_like,pdf,logpdf,cdf,mixture_normal} when PlussMa is available
// - pluss::series::from_torch(...) to build a Series from a torch::Tensor, and
//   the Series::tensor() method for the reverse (when PLUSS_ENABLE_TORCH=ON)
//
// For CodeN (text API), this exports:
// - pluss::Note
// - pluss::TokenBatch
// - pluss::make_note(...) / pluss::make_note(...)
//
// For CodeH (time API), this exports:
// - pluss::Timeline / Duration
// - pluss::timeline::make(...) / from_epoch(...) / date_range(...) / print(...)
// - pluss::duration::{nanos,seconds,...,make,range,print}(...)
// - pluss::make_timeline(...) as the permanent flat shortcut for
//   pluss::timeline::make(...)
//
// Unified serialization (core types + supported STL containers):
// - pluss::io::save(...) / pluss::io::load<...>(...)
// - pluss::save(...) / pluss::load<...>(...) aliases
//
// Validation facade (function-style checks):
// - pluss::validate::check(...) / warn(...) / fail(...) / inspect(...) / check_eq(...)
//
// Dynamic union holder:
// - pluss::Type (Series/Note/Timeline/Duration)
// - pluss::Type::invoke("method", args...) for dynamic method-name dispatch on Type
// - pluss::Queue (Series/Note/Timeline/Duration)
// - pluss::Queue::invoke("method", args...) for dynamic method-name dispatch on Queue
// - pluss::dispense<R>(obj, "method", args...) typed dynamic dispatch
// - pluss::dispense_any(obj, "method", args...) runtime variant dispatch
//
// Core pluss surface (Series/Note/Timeline/Duration + Type/Dispense) lives in PlussQit.
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
//   - pluss::tils::transmit(...), pluss::tils::transform_axis(...)
//   - pluss::loader::DataLoader + pluss::loader::* helpers
//   - pluss::tils::to_scalar<T>(series) + pluss::tils::as_f64/as_f32/as_i64/... helpers
//   - pluss::view::* utilities

#if __has_include("pluss/mic.h")
#include "pluss/mic.h"
#endif

#if __has_include("pluss/pl.h")
#include "pluss/pl.h"
#endif
