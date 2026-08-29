# `lymph::models::Builder` Introduction

## What This Type Represents

`Builder` is the candidate registry you reach for before spotchecking,
benchmarking, or search.

It solves a coordination problem, not a fitting problem:

- stable model names
- duplicate protection
- registry inspection
- remembered comparison outcomes

## Why It Lives Under `models::builder`

This object still belongs to the `models` family because it owns candidate
model identity. The `builder` subfamily exists only to narrow that concern from
"one trainable artifact" to "many candidate artifacts under one named
workspace."

## Reading Order

1. `inject`
2. `has`
3. `get`
4. `report`
5. `benchmark`
6. `best`
7. `worst`

## Related Pages

- [builder subfamily](../Introduction.md)
- [Model type](../../Model.md)
- [Builder notes](Notes.md)
