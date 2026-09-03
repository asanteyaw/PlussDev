# Lymph `io`

`io` is the Lymph family for persistence, restore, reproducibility, and saved
workflow artifacts.

Use this branch when you are deciding:

- how to save and reload trained models or workflows
- what persistence guarantees exist
- how artifacts fit into larger Procedure-based execution

## Why This Family Exists

Training a model is only part of the job.

Sooner or later every serious project asks:

- how do I save this artifact?
- what exactly gets saved?
- can I trust the loaded object to behave the same way?
- where does this stop being a modeling concern and become a deployment concern?

The `io` family answers those questions.

## What Gets Persisted Conceptually

At a high level, Lymph persistence is about preserving the modeling artifact that
matters, not just arbitrary in-memory state.

Depending on the object, that can include:

- learned model state
- prep-stage state
- workflow-level structure
- policy sidecars
- runtime compatibility metadata

This is why the `io` docs matter even for users who think they only care about
training. If the artifact cannot be saved and restored honestly, the project still
has a deployment gap.

## Save Versus Load

### Save

Saving answers:

"how do I turn this trained object into a reusable artifact?"

### Load

Loading answers:

"how do I restore that artifact later without retraining it?"

These are separate concerns because persistence quality is judged not just by
whether save succeeds, but by whether load produces a faithful and compatible
object.

## Relationship To `Workflow`

Persistence becomes especially important once you use `Workflow`.

Why:

- a saved `Model` is a trained engine
- a saved `Workflow` is a trained engine plus its runtime contract

If the deployment boundary matters, the workflow artifact is often the more honest
thing to persist.

## Relationship To `procedure`

Lymph persistence is about model/workflow artifact ownership.

Procedure persistence is about broader pipeline orchestration and component routing.

The two meet, but they do not own the same abstraction layer.

That distinction is useful because it prevents a modeling user from needing the
full Procedure machinery just to save a trained learner or workflow artifact.

## How To Read This Family

Use the pages in this order:

1. [persistence](io/persistence.md) for the contract
2. [save](io/save.md) for artifact emission
3. [load](io/load.md) for restoration

That order moves from concept, to write path, to read path.

## Main Objects

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

io/save
io/load
io/persistence
```
<!-- apiref:toctree:end -->
