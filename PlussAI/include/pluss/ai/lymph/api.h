#pragma once

// Umbrella header for pluss::ai::lymph — kept for backward compatibility with
// every existing `#include "pluss/ai/lymph/api.h"` call site. The actual
// content lives in the per-concern headers below (see PlussAI/Lymph for the
// matching .cpp translation units); this file only sequences their includes
// in dependency order.
//
// Module map:
//   common.h         - resolve(), small free helpers, umbrella includes
//   prep.h           - pluss::ai::lymph::prep (imputers/scalers/encoders)
//   metrics.h        - MetricReport, holdout/cross-validation plumbing
//   sparse.h         - pluss::ai::lymph::sparse (sparse linear models)
//   inspection.h     - pluss::ai::lymph::inspection (PDP/permutation importance)
//   feature_select.h - pluss::ai::lymph::feature (ranking/selection)
//   workflow_model.h - WorkflowModel + cross-validation orchestration
//   workflow_dsl.h   - workflow::feature/stage branch DSL, FeatureTransformer builders
//   workflow.h       - Workflow (staged pipeline facade over WorkflowModel)
//   factories.h      - linear_factory/tree_factory/.../decompose_factory
//   online.h         - OnlineLinearClassifier/Regressor
//   selection.h      - pluss::ai::lymph::selection (model/hyperparameter tuning)

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/prep.h"
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
