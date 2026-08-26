#pragma once

// Umbrella header for pluss::ai::lymph — kept for backward compatibility with
// every existing `#include "pluss/ai/lymph/api.h"` call site. The actual
// content lives in the per-concern headers below (see PlussAI/Lymph for the
// matching .cpp translation units); this file only sequences their includes
// in dependency order.
//
// Module map:
//   common.h         - resolve(), small free helpers, umbrella includes
//   selection_options.h - selection-owned validation option contracts
//   columnrouter.h   - prep::Route + lymph::prep::route:: (names/where/remaining_of)
//   balance.h        - lymph::balance:: factories over NodeJ's existing BalanceTarget
//   prep.h           - pluss::ai::lymph::prep (imputers/scalers/encoders)
//   stage.h          - prep::Stage + lymph::prep::stage:: (Model::append()'s attachable-stage descriptor)
//   transform.h      - TargetTransform + lymph::transform:: (log1p/sqrt/custom factories)
//   metrics.h        - MetricReport, holdout/cross-validation plumbing
//   sparse.h         - pluss::ai::lymph::models::sparse (sparse linear models)
//   inspection.h     - pluss::ai::lymph::inspection (PDP/permutation importance)
//   feature_select.h - pluss::ai::lymph::features (ranking/selection)
//   detail/model_core.h - internal Model implementation, included through model.h
//   workflow_dsl.h      - prep::branch DSL + prep::feature_transformer builders
//   policy.h            - workflow::Policy + lymph::workflow::policy::Policy raw-frame admission/schema rules
//   time.h              - workflow::TimeOptions + row-step duration helpers for temporal Workflow safety
//   payload.h           - workflow::Payload + lymph::workflow::payload::make(...) typed data/metadata carrier
//   workflow.h          - workflow::Workflow + lymph::workflow::make/load composition object
//   factories.h         - internal detail::*_factory construction tags for model.h
//   online.h            - OnlineLinearClassifier/Regressor + lymph::models::online::linear_*
//   selection.h         - pluss::ai::lymph::selection (model/hyperparameter tuning)
//   model.h             - public Model handle + lymph::models:: grouped factories
//   reports.h           - pluss::ai::lymph::reports (model-backed report builders)
//   builder.h           - Builder (Nexus-inspired named model coordinator) + lymph::models::builder::
//   validate.h          - validate::cross/stratified/grouped/time_ordered validation plans
//   spotcheck.h         - spotcheck::suite(Builder) strategy x model comparison
//   unsupervised.h      - cluster/embed result families over transform-style models
//   shape.h             - distribution profile/plan family for feature shape transforms
//   detect.h            - anomaly/outlier detection result family
//   semi.h              - semi-supervised result family over classification models
//   search.h            - Search/search::Result (Model-aware hyperparameter search) + lymph::search::
//
// workflow.h note: this file used to hold a lazy `class Workflow` facade
// (Candidate/CompareResult's optional model-holding wrapper); that facade was
// retired once Model::append(Stage, ColumnRouter) and Candidate/CompareResult
// holding Model directly made it unnecessary. The public Workflow name now
// belongs only to the deployable Model + Policy artifact.
//
// Redesign in progress (see PlussAI/Lymph/LYMPH_REDESIGN_PLAN.md) -- this
// remaining per-family skeleton is not wired into the umbrella yet.
//   select.h         -> Comparison/SelectionResult/Candidate, replaces selection::compare

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/selection_options.h"
#include "pluss/ai/lymph/columnrouter.h"
#include "pluss/ai/lymph/balance.h"
#include "pluss/ai/lymph/prep.h"
#include "pluss/ai/lymph/stage.h"
#include "pluss/ai/lymph/transform.h"
#include "pluss/ai/lymph/metrics.h"
#include "pluss/ai/lymph/sparse.h"
#include "pluss/ai/lymph/inspection.h"
#include "pluss/ai/lymph/feature_select.h"
#include "pluss/ai/lymph/workflow_dsl.h"
#include "pluss/ai/lymph/policy.h"
#include "pluss/ai/lymph/time.h"
#include "pluss/ai/lymph/payload.h"
#include "pluss/ai/lymph/workflow.h"
#include "pluss/ai/lymph/factories.h"
#include "pluss/ai/lymph/online.h"
#include "pluss/ai/lymph/selection.h"
#include "pluss/ai/lymph/model.h"
#include "pluss/ai/lymph/reports.h"
#include "pluss/ai/lymph/builder.h"
#include "pluss/ai/lymph/validate.h"
#include "pluss/ai/lymph/spotcheck.h"
#include "pluss/ai/lymph/unsupervised.h"
#include "pluss/ai/lymph/shape.h"
#include "pluss/ai/lymph/detect.h"
#include "pluss/ai/lymph/semi.h"
#include "pluss/ai/lymph/search.h"
