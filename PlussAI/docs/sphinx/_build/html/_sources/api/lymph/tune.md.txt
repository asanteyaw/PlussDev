# Lymph `tune`

`tune` is the Lymph family for shortlist construction, spotchecking, comparison,
search, and model-selection workflow.

Use this branch when you are deciding:

- how to compare many candidate models quickly
- how to spotcheck strategy/model combinations
- how to move from search space to chosen workflow

## What This Family Owns

The `tune` family is where Lymph answers:

"out of several plausible modeling directions, which one should survive?"

That can mean several different tasks:

- compare several already-built models
- benchmark one builder under several strategies
- search a hyperparameter space
- inspect the report that explains why a winner won

The family exists because model selection is a real stage of work, not merely a few
temporary loops before the "real modeling" begins.

## Distinguish The Three Big Jobs

### Comparison

Comparison asks:

"I already have several candidate models. Which one performs best?"

This is where `Builder`, `Suite`, and `CompareReport` shine.

### Search

Search asks:

"Within this configuration space, which trial wins?"

This is where `Search` belongs.

### Reporting

Reporting asks:

"How do I inspect the evidence and ranking after comparison or search?"

This is where `CompareReport` and `TuneReport` matter.

Keeping those jobs separate is what stops the tuning surface from collapsing into
one opaque "do everything" call.

## Why This Family Matters

Most projects do not fail because a learner could not be instantiated.

They fail because selection was weak:

- only one model family was tried
- the wrong validation objective decided the winner
- search explored the wrong region
- the evidence board was too thin to audit

The `tune` family is the part of Lymph that forces those decisions into the open.

## Relationship To Other Families

`tune` depends heavily on:

- `model`, because models are what get compared
- `validate`, because validation policy determines what "winning" even means
- `metrics`, because the winner is defined through objective metrics

So tuning is not isolated. It is the convergence point where modeling, validation,
and scoring meet.

## Typical Progression Through This Family

A strong workflow often looks like:

1. build a named shortlist with `Builder`
2. run `Suite` for broader spotchecking or strategy comparison
3. inspect the resulting comparison report
4. move the strongest direction into `Search`
5. promote the winning configuration into a final `Model` or `Workflow`

That is a much more reliable progression than jumping straight into a huge search
space without first checking whether the broad model family is even competitive.

## Main Objects

```{toctree}
:maxdepth: 2

tune/Suite
tune/Search
tune/CompareReport
tune/TuneReport
```
