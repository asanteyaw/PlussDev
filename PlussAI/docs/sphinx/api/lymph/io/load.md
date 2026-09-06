# `load`

## Summary

The Lymph load surface restores a previously saved model or workflow artifact.

Use it when you want reproducible reload of a trained artifact rather than
reconstructing and retraining from scratch.

## What "Load" Means In Practice

Loading is the restoration half of the persistence contract.

It answers:

- can I bring this artifact back into memory?
- does the restored object behave like the saved one is meant to behave?
- can I resume inference or assessment without retraining?

If save is about artifact emission, load is about artifact trust.

## Signature

```cpp
lymph::Model lymph::Model::load(const std::string& path);
lymph::Workflow lymph::workflow::load(const std::string& path);
```

## Typical Load Shapes

Model-style restoration:

```cpp
lymph::Model model = lymph::Model::load("artifacts/logistic_model");
```

Workflow-style restoration:

```cpp
lymph::Workflow wf = lymph::workflow::load("artifacts/default_workflow");
```

### Output

Copied from an executed run of
`plussai_lymph_model_prep_io_docverify_demo`:

```text
== io ==
model_roundtrip=1
workflow_roundtrip=1
loaded_workflow_trained=1
```

This is the load contract in the form that matters:

- the loaded model reproduces the same predictions
- the loaded workflow reproduces the same predictions
- the workflow still knows it is a trained artifact after reload

Choose the load path that matches the artifact you saved. A workflow artifact
should come back as a workflow artifact.

## What To Check After Loading

After loading, the first questions should be:

- is this the right artifact type?
- was it trained/prepared as expected before saving?
- do the runtime expectations still match the current data path?

For example, if the loaded object is a workflow, the policy contract remains part of
what you should reason about during reuse.

## Why Load Is Better Than Rebuild For Reuse

Rebuilding and retraining may sound harmless, but it introduces drift:

- different random states
- different prep state
- different data snapshot
- different fold outcomes

Loading is what preserves the actual chosen artifact rather than a new imitation of
it.

## Related Pages

- [save](save.md)
- [persistence](persistence.md)
- [Workflow](../workflow/Workflow.md)
