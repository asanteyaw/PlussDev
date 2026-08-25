# Lymph `model`

`model` is the Lymph family for the durable objects users actually carry through a
project.

This is where you answer questions like:

- what object represents the thing I am training?
- when is a plain `Model` enough?
- when should I move into `Workflow`?
- where do `Policy`, `Payload`, and `Builder` fit around the model?

If `validate` is the family that defines how evidence is earned, `model` is the family
that defines what is being evaluated in the first place.

## What This Family Owns

The `model` family owns the main left-hand-side objects of a Lymph project:

- `Model`
- `Workflow`
- `Builder`
- `Policy`
- `Payload`

These are not helper structs in the background. They are the public surface that
keeps experiments, training flows, and deployment-ready artifacts structured.

## What Makes This Family Important

The quality of a modeling API is not just about how many estimators it exposes.

It is also about whether the user can represent a project cleanly:

- one object for the staged trainable pipeline
- one object for payload intent
- one object for benchmark construction
- one object for richer workflow composition

That is the job of this family.

## Main Objects

```{toctree}
:maxdepth: 2

model/Model
model/Workflow
model/Builder
model/Policy
model/Payload
```

## Recommended Reading Order

1. [Model](model/Model.md)
2. [Workflow](model/Workflow.md)
3. [Payload](model/Payload.md)
4. [Builder](model/Builder.md)
5. [Policy](model/Policy.md)

That order mirrors how users normally progress from one trained object, to reusable
workflow, to richer orchestration.
