#pragma once

// select.h -- Comparison / SelectionResult / Candidate.
//
// Role: ranks whole pre-built candidates (family x balance-strategy
// combinations, or any other Model variants the caller already built)
// against each other via cross-validation. Replaces selection.h's
// selection::compare -- same underlying (candidate x fold) parallel
// dispatch this session already built and tested, wrapped as an object
// instead of a free function.
//
// Originally scoped as staying a plain function (a candidate list looked
// like a one-shot comparison, not reusable config the way a Search axis
// list is) -- reversed once it became clear that asymmetry with Search is
// something a caller has to remember, not predict. See
// LYMPH_REDESIGN_PLAN.md 3.3/6.
//
// What lands here:
//   - struct Candidate { std::string name; Model model; TrainOptions
//     options; } -- an aggregate, designated-init at the call site
//     (candidates.push_back({.name=..., .model=..., .options=...})).
//   - class Comparison -- holds a vector<Candidate>; .run(batch,
//     data_options, cv_options, ...) -> SelectionResult.
//   - class SelectionResult -- .board() -> DataFrame; .top(k) ->
//     vector<pair<string, Model>>; .best() -> Model. Same shape as
//     search.h's search::Result -- learn one, know both.
//   - namespace lymph::select { Comparison candidates(vector<Candidate>); }
//
// Depends on: model.h (Model), the existing selection.h compare_batch_
// dispatch engine (reused, not rewritten -- this file wraps it, doesn't
// replace the underlying parallel dispatch. See Lymph/src/select.cpp).
//
// Consumed by: nothing yet -- terminal (feeds .best()/.top(k) into
// pipeline.h by hand).

namespace pluss::ai::lymph {

// Candidate, Comparison, SelectionResult, and lymph::select:: factory
// namespace land here.

} // namespace pluss::ai::lymph
