# Lymph `metrics`

`metrics` is the Lymph family for evaluation outputs, metric helpers, validation
contracts, threshold inspection, and workflow-facing report surfaces.

Use this branch when you are deciding:

- how to score classification or regression outputs
- how to interpret validation reports
- how to inspect thresholds and summary tables after inference

## What This Family Owns

The `metrics` family is where raw predictive outcomes are turned into interpretable
evidence.

That includes:

- validation options
- metric-value summaries
- cross-validation reports
- confusion tables
- classification summaries
- threshold inspection surfaces

In other words, this is not just "the math after inference." It is the part of the
API that tells you whether a modeling result is actually worth trusting.

## Why This Family Matters

A modeling API is only as good as the honesty of its evaluation surface.

Without a strong `metrics` family, users end up with:

- one-off score calculations
- half-tracked fold results
- confusion tables computed differently in different scripts
- thresholds chosen without inspection

The `metrics` family solves that by keeping evaluation artifacts structured and
reusable.

## The Main Evaluation Layers

### Scalar and vector metrics

These answer questions like:

- what is the accuracy?
- what is the F1?
- what is the RMSE?
- how do fold scores vary?

### Report objects

These answer higher-level questions like:

- what are the mean and standard deviation across folds?
- which classes are failing?
- where is the threshold tradeoff acceptable?

### Presentation-ready tables

Objects such as confusion tables and classification summaries exist because teams
often need artifacts that can be inspected, exported, or attached to reports.

## Relationship To Other Families

`metrics` works closely with:

- `validate`, which decides how evidence is earned
- `model`, which produces predictions
- `tune`, which uses metrics to rank candidates and search trials

That relationship is important:

- `metrics` owns the validation options bag
- `validate` consumes that contract to build executable plans
- `metrics` describes the evidence that contract produced

## How To Read This Family

Use this family in roughly this order:

1. understand fold-oriented reporting with `CrossValidationReport`
2. inspect class behavior with `confusion_table` and `classification_summary`
3. move to threshold analysis when operating points matter

That order mirrors real model review.

First ask whether the model is broadly good.
Then ask where it fails.
Then ask where to place the operating threshold.

## Main Objects

```{toctree}
:maxdepth: 2

metrics/ValidationOptions
metrics/Threshold
metrics/CrossValidationReport
metrics/confusion_table
metrics/classification_summary
```
