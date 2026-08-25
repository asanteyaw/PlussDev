# `lymph::selection::TuneReport`

## Summary

`TuneReport` is the structured output of tuning or sweep-style model selection in
Lymph.

Use it when you need the search history, winning configuration, and supporting
metrics in one object.

## What It Represents

`TuneReport` is the search-history artifact.

Where `CompareReport` is about ranking a set of named finished candidates,
`TuneReport` is about ranking trial points inside a tuning or sweep process.

That means it is the right object when the key question is:

"which parameter configuration won, and what did the rest of the search space look
like?"

## Why It Matters

Tuning should never be treated as a black box that emits a winner and discards the
path taken to get there.

A useful tuning report preserves:

- the objective history
- the trial ordering
- parameter encodings or traces
- the winning configuration

This is what lets users review not just the result, but the search behavior.

## Relationship To Search

`Search` is the execution surface.
`TuneReport` is the evidence artifact.

That split is healthy because it keeps the search API focused on running trials
while leaving inspection to a report object purpose-built for that job.

## Signature

```cpp
lymph::TuneReport report{};
pluss::Series lymph::objective_series(const lymph::TuneReport& report);
pluss::Series lymph::objective_rank_series(const lymph::TuneReport& report);
```

## How To Use It

Use a tuning report when you want to answer:

- was the best result clearly better or only marginally better?
- did the objective stabilize or remain noisy?
- which region of the search space looked promising?
- what should I try next if another search pass is needed?

## Related Pages

- [Search](Search.md)
- [CompareReport](CompareReport.md)
