# `prep::stage` Introduction

## What This Subfamily Owns

`prep::stage` owns the factories that create attachable stage values.

Typical examples are:

- `prep::stage::standard_scaler(...)`
- `prep::stage::one_hot_encoder(...)`

These factories do not fit data themselves. They describe what should be
attached to a model or feature transformer later.

## Why It Is Separate From Standalone Transformers

This split is important:

- `prep::StandardScaler` is a standalone fitted object
- `prep::stage::standard_scaler(...)` is an attachable stage descriptor

They take related options, but they play different roles in the system.

## Main Entry Points

- [standard_scaler](standard_scaler.md)
- [one_hot_encoder](one_hot_encoder.md)
- [Stage](Stage.md)
