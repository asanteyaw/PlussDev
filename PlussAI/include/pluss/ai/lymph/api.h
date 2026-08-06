#pragma once

// Umbrella header for pluss::ai::lymph — kept for backward compatibility with
// every existing `#include "pluss/ai/lymph/api.h"` call site. The actual
// content lives in the per-concern headers below (see PlussAI/Lymph for the
// matching .cpp translation units); this file only sequences their includes
// in dependency order.
//
// Module map:
//   common.h         - resolve(), small free helpers, umbrella includes
//   columnrouter.h   - ColumnRouter + lymph::columnrouter:: (names/where/remaining_of)
//   balance.h        - lymph::balance:: factories over NodeJ's existing BalanceTarget
//   prep.h           - pluss::ai::lymph::prep (imputers/scalers/encoders)
//   stage.h          - Stage + lymph::stage:: (Model::append()'s attachable-stage descriptor)
//   transform.h      - TargetTransform + lymph::transform:: (log1p/sqrt/custom factories)
//   metrics.h        - MetricReport, holdout/cross-validation plumbing
//   sparse.h         - pluss::ai::lymph::sparse (sparse linear models)
//   inspection.h     - pluss::ai::lymph::inspection (PDP/permutation importance)
//   feature_select.h - pluss::ai::lymph::feature (ranking/selection)
//   workflow_model.h    - WorkflowModel + cross-validation orchestration
//   workflow_dsl.h      - workflow::feature/stage branch DSL, FeatureTransformer builders
//   workflow.h          - (skeleton only, see below) reserved for the future Workflow deployable
//   factories.h         - linear_factory/tree_factory/.../decompose_factory
//   online.h            - OnlineLinearClassifier/Regressor
//   selection.h         - pluss::ai::lymph::selection (model/hyperparameter tuning)
//   frame_prep.h         - FramePrepPolicy, PreparedModel (replayable raw-frame column-drop/null-filter)
//   model.h             - Model (= WorkflowModel alias) + lymph::model:: (consolidated factories)
//   builder.h           - Builder (Nexus-inspired named model coordinator) + lymph::builder::
//   search.h            - Search/SearchResult (Model-aware hyperparameter search) + lymph::search::
//
// workflow.h note: this file used to hold a lazy `class Workflow` facade
// (Candidate/CompareResult's std::optional<WorkflowModel>-holding wrapper);
// that facade was retired once Model::append(Stage, ColumnRouter) and
// Candidate/CompareResult holding WorkflowModel directly made it
// unnecessary (LYMPH_REDESIGN_PLAN.md progress tracker has the full
// writeup). The file (and its "Workflow" name) is reserved for a second,
// unrelated thing: the eventual rename of today's PreparedModel
// (frame_prep.h) via pipeline.h's plan -- "Pipeline" retired in favor of
// "Workflow" per the user's request, pipeline.h's skeleton merged into
// workflow.h's header comment and pipeline.h deleted. Not yet implemented.
//
// Redesign in progress (see PlussAI/Lymph/LYMPH_REDESIGN_PLAN.md) -- the
// per-family files below exist as comment-only skeletons and are NOT yet
// wired into this umbrella; every file above stays the live, working
// surface until each new file's real content lands and callers migrate.
//   select.h         -> Comparison/SelectionResult/Candidate, replaces selection::compare
//   policy.h         -> Policy, renames FramePrepPolicy + lymph::policy(...)
//   workflow.h       -> Workflow, renames PreparedModel + lymph::workflow:: (see note above)

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/columnrouter.h"
#include "pluss/ai/lymph/balance.h"
#include "pluss/ai/lymph/prep.h"
#include "pluss/ai/lymph/stage.h"
#include "pluss/ai/lymph/transform.h"
#include "pluss/ai/lymph/metrics.h"
#include "pluss/ai/lymph/sparse.h"
#include "pluss/ai/lymph/inspection.h"
#include "pluss/ai/lymph/feature_select.h"
#include "pluss/ai/lymph/workflow_model.h"
#include "pluss/ai/lymph/workflow_dsl.h"
#include "pluss/ai/lymph/workflow.h"
#include "pluss/ai/lymph/factories.h"
#include "pluss/ai/lymph/online.h"
#include "pluss/ai/lymph/selection.h"
#include "pluss/ai/lymph/frame_prep.h"
#include "pluss/ai/lymph/model.h"
#include "pluss/ai/lymph/builder.h"
#include "pluss/ai/lymph/search.h"
