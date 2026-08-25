# Lymph persistence

## Summary

This page describes the persistence contract around Lymph artifacts at a high level.

Use it when you want to understand:

- what gets saved
- what kind of reproducibility is intended
- where Lymph persistence meets Procedure persistence

## The Core Persistence Promise

Lymph persistence is meant to preserve a modeling artifact faithfully enough that
the artifact can be restored and reused without retraining.

That does not mean "every possible runtime situation is automatically compatible
forever."

It means:

- the artifact carries its learned state and owned modeling contract
- the restore path is explicit
- compatibility and artifact ownership are treated seriously

## Signature

```cpp
void lymph::Model::save(const std::string& path) const;
lymph::Model lymph::Model::load(const std::string& path);
void lymph::Workflow::save(const std::string& path) const;
lymph::Workflow lymph::workflow::load(const std::string& path);
```

## What Persistence Is For

Use persistence when the project needs:

- repeatable inference
- artifact handoff between training and deployment steps
- long-running experiment preservation
- reproducible evaluation on a chosen fitted object

If the project only needs an in-memory experimental object for the current process,
save/load may be unnecessary. The moment the artifact must outlive the process,
persistence stops being optional.

## Model Artifact Versus Workflow Artifact

This distinction is one of the most important in the whole Lymph documentation.

### Model artifact

A saved `Model` preserves the trainable pipeline artifact.

That is appropriate when the model itself is the meaningful reusable unit.

### Workflow artifact

A saved `Workflow` preserves the trainable pipeline plus the runtime contract around
it.

That is appropriate when the deployable behavior matters, not just the fitted
learner state.

In many production situations, the workflow artifact is the stronger truth.

## End-To-End Roundtrip Example

```cpp
lymph::Workflow workflow = lymph::workflow::make(model, policy);
workflow.save("artifacts/default_workflow");

lymph::Workflow loaded =
    lymph::workflow::load("artifacts/default_workflow");
Series predicted = loaded.infer(incoming, data_options);
```

### Output

Copied from an executed run of
`plussai_lymph_model_prep_io_docverify_demo`:

```text
== workflow ==
predictions=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 0]
roundtrip_match=1
== io ==
model_roundtrip=1
workflow_roundtrip=1
loaded_workflow_trained=1
```

This is the persistence promise rendered concretely. The saved workflow comes back
as the same trained runtime contract, not just a vaguely similar object.

## Persistence And Reproducibility

Persistence improves reproducibility because it lets you reuse the actual selected
artifact rather than reconstructing it from memory.

But persistence is not magic.

Good reproducibility still depends on:

- disciplined artifact versioning
- stable save paths and naming
- explicit runtime contracts
- clear distinction between training-time and deployment-time objects

Lymph gives you the persistence surface; teams still need sound artifact hygiene.

## Relationship To Procedure

Lymph persistence and Procedure persistence are related but not identical.

- Lymph owns the model/workflow artifact layer
- Procedure owns the broader multi-component orchestration layer

That means a user can work productively with Lymph persistence without needing the
full Procedure system, while Procedure can still build on top of Lymph artifacts
when the project grows.

## Related Pages

- [save](save.md)
- [load](load.md)
- [PlussAI `procedure`](../../plussai_procedure.md)
