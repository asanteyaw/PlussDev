#pragma once

// workflow.h -- Workflow (renamed from PreparedModel, frame_prep.h; the name
// "Workflow" is renamed a second time in this file's history -- see below).
//
// This file used to hold a different, now-retired `class Workflow`: a lazy
// `std::optional<WorkflowModel>` + `std::vector<TransformSpec>` facade that
// existed so a Candidate could carry stages before any Model existed to
// attach them to. `Model::append(Stage, ColumnRouter)` (stage.h,
// workflow_model.h's `pending_stages_` mechanism) replaced the need for that
// laziness outright -- every TransformKind already has its own direct
// WorkflowModel builder method (`.standard_scaler(...)`, `.simple_imputer(
// ...)`, ...) that attaches immediately, no facade required -- and
// `selection::Candidate`/`CompareResult` now hold `WorkflowModel` directly
// (LYMPH_REDESIGN_PLAN.md's "retire the Workflow lazy facade" phase). Every
// real caller in this repo migrated off it onto WorkflowModel's own methods
// directly; the old class, `make_pipeline`/`pipeline(...)`, the
// `TrainedHandle`/`AnyTrainedModel` aliases, and its `cross_validate(
// Workflow, ...)` overloads were deleted once nothing referenced them
// (confirmed by a repo-wide grep before deletion) -- see
// LYMPH_REDESIGN_PLAN.md's progress tracker for the migration writeup.
//
// The name "Workflow" is reused here for a second, unrelated thing (the
// redesign's own naming churn: "Pipeline" -> "Workflow" per the user's
// explicit request, since the old lazy facade no longer needs the name):
// what pipeline.h used to describe as `Pipeline` (itself a planned rename of
// today's `PreparedModel`, frame_prep.h) lands here once implemented --
// pipeline.h's skeleton has been merged into this comment and the file
// deleted; nothing is implemented yet.
//
// Role: the actual "stack everything, raw data in, prediction out"
// deployable object -- Model + Policy, save()/load() round-tripping both
// together via a small sidecar next to the model bundle. This is the object
// the very first question in this whole redesign thread ("the final stage
// for deployment is the pipeline that stacks everything") was asking about.
// Unchanged in behavior from today's PreparedModel; renamed twice (Pipeline,
// then Workflow), and its construction moves off a two-argument constructor
// call into lymph::workflow::from(model, policy) / lymph::workflow::load(
// path), per the construction-vs-method rule -- Workflow::load(...)
// specifically can't just be lowercased, since a static method forces the
// class's exact name on the left of `::` (LYMPH_REDESIGN_PLAN.md 2.0 rule
// 2).
//
// What lands here (not yet implemented):
//   - class Workflow -- learn_/infer/assess/save, all (frame,
//     DataFrameBatchOptions) shaped, each running Policy::apply(frame)
//     first (was: apply_frame_prep(frame, policy_) called internally --
//     unchanged, just reads as a method call on policy_ now).
//   - save_policy(path, Policy) / load_policy(path) -- the sidecar
//     persistence helpers (renamed from save_frame_prep_policy/
//     load_frame_prep_policy in frame_prep.h), moved here since they're
//     Workflow-only concerns, not something Policy itself needs to know
//     about.
//   - namespace lymph::workflow { Workflow from(Model, Policy); Workflow
//     load(std::string path); }
//
// Depends on: model.h (Model), policy.h (Policy), NodeJ's io::save/
// io::load/FlowBundle (unchanged -- the sidecar file is intentionally
// outside FlowBundle's own schema, same reasoning as today's frame_prep.h).
// See Lymph/src/workflow.cpp for save/load's own logic once it lands.
//
// Consumed by: nothing -- this is the end of the lifecycle.

namespace pluss::ai::lymph {

// Workflow class, save_policy/load_policy helpers, and lymph::workflow::
// factory namespace land here.

} // namespace pluss::ai::lymph
