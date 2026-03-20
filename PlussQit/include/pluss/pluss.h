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
// Dynamic union holder:
// - pluss::Queue (Series/Note/Timeline/TimeIndex)
// - pluss::Queue::invoke("method", args...) for dynamic method-name dispatch on Queue
// - pluss::dispense<R>(obj, "method", args...) typed dynamic dispatch
// - pluss::dispense_any(obj, "method", args...) runtime variant dispatch
//
// Aux algorithm utilities:
// - pluss::transmit(...) for elementwise map/zip transforms over values/ranges
// - pluss::transform_axis(...) for NumPy-style apply_along_axis slice transforms
// - pluss::DataLoader convenience alias (single root-level export)
// - pluss::loader::DataLoader for Series-native mini-batch iteration (shuffle/drop_last)
// - pluss::loader::Dataset / pluss::loader::SeriesDataset map-style dataset surfaces
// - pluss::loader::split_fields(...) / pluss::loader::split_loaders(...) for train/val/test partitioning
// - pluss::loader::time_series_split_indices/fields/loaders(...) for sklearn-like TimeSeriesSplit workflows
// - pluss::to_scalar<T>(series), pluss::as_f64/as_f32/as_i64/... scalar extract helpers
// - pluss::loader::set_path_root(...), pluss::loader::path_root(...), pluss::loader::path_file(...), plus data/output/model helpers
//
// Core pluss surface (Series/Note/Timeline/TimeIndex + Queue/Dispense/Aux) lives in PlussQit.
#include "pluss/core.h"

// PlussAI is intentionally opt-in:
// - include <pluss/nn_func.h> for lightweight functional neural surfaces
// - include <pluss/nn.h> for full neural module stack
// - include <pluss/ai.h> for neural + classical ML (NodeJ) surface
//
// PlussMa is intentionally opt-in:
// - include <pluss/ma.h> for PlussMa math/stat/time-series utilities

#if __has_include("pluss/mic.h")
#include "pluss/mic.h"
#endif

#if __has_include("pluss/pl.h")
#include "pluss/pl.h"
#endif
