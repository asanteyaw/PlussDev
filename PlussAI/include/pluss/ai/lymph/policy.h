#pragma once

// policy.h -- Policy (renamed from FramePrepPolicy, frame_prep.h).
//
// Role: declarative, replayable raw-frame cleanup -- which columns never
// reach a model (ID/leakage columns) and which columns must be non-null for
// a row to survive. Unchanged in behavior from today's FramePrepPolicy
// (built and tested this session); this file's job is the rename
// (LYMPH_REDESIGN_PLAN.md 2.0 rule 6: "any short, one-word descriptive
// name") and moving construction off a bare aggregate literal into the
// lymph::policy(...) factory, per the construction-vs-method rule.
//
// What lands here:
//   - struct Policy (renamed from FramePrepPolicy, fields unchanged:
//     drop_columns, filter_nulls, null_filter_columns,
//     drop_only_if_all_null).
//   - Policy::apply(DataFrame) -> DataFrame -- was the free function
//     apply_frame_prep(frame, policy); now a method, since "apply myself to
//     a frame" is obviously Policy's own operation once a Policy value
//     already exists (construction-vs-method rule 1).
//   - lymph::policy({...}) -- a single free function, not a
//     lymph::policy:: namespace: Policy has only one construction shape, no
//     sub-variants to distinguish (unlike ColumnRouter/Model/Search, which
//     each have several named factories).
//
// What does NOT land here: save_frame_prep_policy/load_frame_prep_policy
// (the bundle sidecar persistence) -- those move to pipeline.h, since only
// Pipeline::save/load ever calls them; Policy itself has no persistence of
// its own.
//
// Depends on: DataFrame::drop()/dropnull() only (pluss::table) -- nothing
// NodeJ-level, unchanged from today's frame_prep.h.
//
// Consumed by: pipeline.h (Pipeline = Model + Policy).

namespace pluss::ai::lymph {

// Policy struct and lymph::policy(...) factory land here.

} // namespace pluss::ai::lymph
