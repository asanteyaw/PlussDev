# `save`

## Summary

The Lymph save surface persists trained model or workflow artifacts for later reuse.

Use it when you want the trained state to leave the current process and survive into
future inference or evaluation runs.

## What "Save" Means In Lymph

Saving is not just serialization for its own sake.

It is the act of freezing a modeling artifact so it can be:

- reloaded later
- shipped to another runtime path
- used for inference without retraining
- audited as the concrete artifact that actually went into use

This is why saving is a first-class documented surface rather than an afterthought.

## Signature

```cpp
void lymph::Model::save(const std::string& path) const;
void lymph::Workflow::save(const std::string& path) const;
void lymph::Workflow::save(
    const std::string& path,
    const lymph::Payload& payload,
    const lymph::TrainOptions& train_options = {},
    pluss::aux::Structured metadata = {}) const;
```

## Typical Save Targets

The most common save targets are:

- `Model`
- `Workflow`

Which one you save depends on what you need the artifact to represent.

### Save a `Model`

Use this when the fitted learner pipeline itself is the main artifact.

### Save a `Workflow`

Use this when the runtime contract matters too:

- admission rules
- policy sidecars
- branch/routing structure

In production-oriented projects, saving the workflow is often the more honest
choice.

## Practical Example

```cpp
lymph::Model model = lymph::model::logistic_regression();
model.learn_(batch);
model.save("artifacts/logistic_model");
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

The most important save signal is not “a file was written.” It is “the saved
artifact can be loaded back and behaves the same way.” That is why the docs now
show roundtrip verification rather than only a save call.

Workflow example:

```cpp
lymph::Workflow wf = lymph::workflow::make(model, policy);
wf.learn_(frame, {
    .label_feature = "default",
});
wf.save("artifacts/default_workflow");
```

### Output

```text
workflow_roundtrip=1
loaded_workflow_trained=1
```

## Save Discipline

A few habits make persistence much more reliable:

- save only artifacts that have actually been trained or prepared as required
- save under stable, explicit paths
- treat saved artifacts as versioned outputs, not casual temporary files
- prefer saving the highest-level honest artifact, not the smallest possible one

That last point is particularly important. If policy or workflow structure matters,
saving only the inner model may be too weak a representation of reality.

## Related Pages

- [load](load.md)
- [persistence](persistence.md)
- [Workflow](../workflow/Workflow.md)
