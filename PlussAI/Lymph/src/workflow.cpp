// workflow.cpp -- Workflow::save/load and the policy sidecar
// (save_policy/load_policy).
//
// Expected to hold what's today frame_prep.h's PreparedModel::save/
// PreparedModel::load plus save_frame_prep_policy/load_frame_prep_policy,
// moved out of the header (frame_prep.h currently keeps all of this
// inline) now that workflow.h is a real per-concern file rather than
// something sharing space with Policy. Existing, tested logic
// (lymph_frame_prep_test.cpp already covers the save/reload/re-predict
// round trip) -- a move, not new engineering.
//
// Nothing implemented yet -- placeholder until Workflow's actual API lands
// in workflow.h and this file's logic gets moved in from frame_prep.h.
// (Renamed from pipeline.cpp/pipeline.h -- "Pipeline" is retired in favor of
// "Workflow", reusing the name freed when the old lazy Workflow facade was
// deleted from this same file; see workflow.h's header comment.)
