# `lymph::options::HoldoutSplit`

`options::HoldoutSplit` (family alias `selection::HoldoutOptions`; it is
`prep::HoldoutSplitOptions` underneath) is the option struct behind every
[`HoldoutPlan`](HoldoutPlan.md). [`holdout(options = {})`](holdout.md) hands it
to `HoldoutPlan`, whose chained setters refine it; `HoldoutPlan::options()`
reads it back.

```cpp
struct HoldoutSplitOptions {
  double        test_size        = 0.2;
  Series        stratify         = Series::undefined();
  std::string   stratify_feature = {};
  std::uint64_t random_state     = 0;
  bool          deterministic    = true;
  bool          shuffle          = true;
};
```

## Fields

### `test_size`

`double test_size = 0.2`

Fraction of rows in the test set. Must be strictly between 0 and 1. Setter:
`HoldoutPlan::test_size`.

### `stratify`

`Series stratify = Series::undefined()`

An explicit stratification key. Set by `HoldoutPlan::stratify(Series)`, which
also clears `stratify_feature`.

### `stratify_feature`

`std::string stratify_feature = {}`

A stratification key given by column name. Set by
`HoldoutPlan::stratify(std::string)`, which also clears `stratify`. At most one
of the two is set; leave both empty for an unstratified split (the right choice
for a regression target).

### `shuffle`

`bool shuffle = true`

Shuffle before cutting. With `false`, the test set is simply the last
`test_size` fraction of rows in their current order. Setter:
`HoldoutPlan::shuffle`.

### `random_state`

`std::uint64_t random_state = 0`

The shuffle RNG seed. `HoldoutPlan::seed(n)` sets this and `deterministic`
together.

### `deterministic`

`bool deterministic = true`

Whether the cut is reproducible for a fixed `random_state`.

## Related Pages

- [`HoldoutPlan`](HoldoutPlan.md) — the object these options configure.
- [`holdout`](holdout.md) — the factory.
