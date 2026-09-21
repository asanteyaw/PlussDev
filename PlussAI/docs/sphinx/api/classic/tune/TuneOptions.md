# `pluss::ai::classic::TuneOptions`

The whole search-strategy contract for [`tune`](tune.md)'s two overloads —
which strategy runs, how many trials it costs, how much of it runs
concurrently, which split plan every trial shares, and the RNG behind any
sampling the strategy itself does. [`sweep`](sweep.md) and
[`probe`](probe.md) take their own parameters directly instead of a
`TuneOptions`; this struct exists so [`tune`](tune.md) can expose every
strategy through one options bag rather than one overload per strategy.

## Fields

### `search`

`SearchKind search = SearchKind::None`

Which strategy [`tune`](tune.md) runs — see its own `## Parameters` for the
full dispatch table, one per entry-point shape (candidate list vs.
sampler). `SearchKind` has five values:

- `None` — the default. Candidate-list overload: identical to `Grid`.
  Sampler overload: identical to `Random`.
- `Grid` — candidate-list overload only: every candidate, once each (calls
  [`sweep`](sweep.md)). Rejected on the sampler overload (there is no
  finite list to exhaust).
- `Random` — candidate-list overload: `trials` candidates sampled without
  replacement from the list. Sampler overload: `trials` candidates drawn
  one at a time (calls [`probe`](probe.md)).
- `SuccessiveHalving` — evaluates every candidate, keeps the better half
  each round, until one remains. `trials` is ignored on the candidate-list
  overload (every candidate starts round 0); on the sampler overload,
  exactly `trials` candidates are drawn up front to start round 0.
- `Bayesian` — a Gaussian-process-guided search that evaluates `trials`
  candidates one at a time, using every previous trial's outcome to choose
  the next. Candidate-list overload: chooses among the given list.
  Sampler overload: draws a pool of `max(trials + 8, trials * 4)`
  candidates up front, then searches within that pool.

### `trials`

`std::int64_t trials = 0`

The trial budget. Ignored by `search = Grid` (candidate-list overload) and
by `search = SuccessiveHalving` (candidate-list overload only — the
sampler overload *does* read it, to size the up-front pool). Required
(`> 0`) by every other combination; `tune` returns
`Status::invalid_argument` when a combination that reads it gets `<= 0`.

### `max_parallel`

`std::int64_t max_parallel = 1`

How many trials [`sweep`](sweep.md#parameters) /
[`probe`](probe.md#parameters) run concurrently, forwarded unchanged —
`<= 0` auto-sizes from hardware concurrency, matching their own default
mechanics. Read by every `search` value except `Bayesian`, whose trials
depend on each other's outcomes and always run one at a time regardless of
this field.

### `split`

`SplitOptions split = {}`

Forwarded as the `split` argument to whichever function `tune` dispatches
to — every trial in one `tune` call is cross-assessed against the same
split plan. See [`SplitOptions`](../split/SplitOptions.md) for every field.

### `random`

`RandomState random = {}`

The seed/determinism pair behind whichever part of the chosen `search`
value involves randomness: `Random`'s without-replacement sampling (or
`probe`'s per-trial seed derivation), `Bayesian`'s warm-up-phase candidate
choices, and `SuccessiveHalving`'s up-front pool draw on the sampler
overload. Has no effect on `Grid`/`None` (candidate-list overload) or
`SuccessiveHalving` (candidate-list overload) — neither samples anything.
See [`RandomState`](../core/RandomState.md) for the general
seeded/non-deterministic contract.

## Guidance

Leave `search` at its default `None` to just run every candidate once
(`sweep`/`probe`'s own plain behavior) through the shared `tune` entry
point. Reach for `Random` when the candidate list (or sampler's range) is
too large to exhaust affordably; `SuccessiveHalving` when trials are
expensive and you want to spend most of the budget on the survivors
rather than spread it evenly; `Bayesian` when trials are expensive enough
that spending some of the budget deciding *which* candidate to try next is
worth it. `max_parallel` and `random` only matter for the `search` values
that actually read them — see each field's own entry above.

## See Also

- [`tune`](tune.md) — the only consumer, and the full per-overload
  dispatch table.
- [`sweep`](sweep.md) / [`probe`](probe.md) — the two strategies `tune`
  itself implements directly; every other `search` value is built on one
  of them internally.
- [`SplitOptions`](../split/SplitOptions.md) / [`RandomState`](../core/RandomState.md) —
  the two fields forwarded, documented in full on their own pages.
