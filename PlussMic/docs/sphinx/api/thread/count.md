<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c53b21de66e7 -->

# Thread::count

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::count(bool count_true = true) const -> ThreadValue
```

## What It Does

Counts entries in the thread — but exactly *what* it counts depends on the
payload's dtype:

- **Non-`Bool` payload** (any numeric `Series`, `Note`, `Timeline`, `TimeIndex`):
  `count()` returns the number of **non-missing entries**. `count_true` must
  stay `true` for these payloads — passing `false` throws, since "count the
  falses" has no meaning outside a boolean payload.
- **`Bool` `Series` payload**: `count_true` switches what gets counted —
  `count(true)` (the default) counts `true` values, `count(false)` counts
  `false` values. This is a genuinely different computation from the non-`Bool`
  case, not just a filter on top of it: it counts by truth value, not by
  missingness.

See [`Thread::sum`'s note on printing a `ThreadValue`](sum.md) — every example
below uses `.to_int()` to extract a clean integer instead of streaming the
`ThreadValue` directly.

## Parameters

**count_true** (`bool`, default `true`)
    For a `Bool` `Series`: which truth value to count. For every other payload:
    must stay `true` (the non-missing count); `false` throws.

## Returns

`ThreadValue`
    A scalar `Int64` count.

## Examples

Example 1. On a non-`Bool` payload, `count()` is the non-missing count.

```cpp
using namespace pluss::table;

auto readings = Thread(
    pluss::make_series({1.0, 2.0, 3.0, 4.0}),
    pluss::make_series(std::vector<bool>{true, false, true, true})
);
auto out = readings.count();
std::cout << out.to_int();
```

### Output

```text
3
```

One entry (position `1`) is missing, so the non-missing count is `3`, not the
total length `4`.

Example 2. On a `Bool` `Series`, `count_true` picks which truth value to tally.

```cpp
auto flags = Thread(pluss::make_series(std::vector<bool>{true, false, true, true}));
auto trues = flags.count(true);
auto falses = flags.count(false);
std::cout << "trues: " << trues.to_int() << ", falses: " << falses.to_int();
```

### Output

```text
trues: 3, falses: 1
```

Example 3. `count(false)` on a non-`Bool` payload throws — `Thread`'s own guard
catches it before delegating further, for `Note`/`Timeline`/`TimeIndex` kinds.

```cpp
auto words = Thread(pluss::make_note({"a", "b"}));
try {
  auto bad = words.count(false);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::count(false) is only valid for Bool Series payload
```

## Notes

- For non-`Bool` payloads, `count()` is always the non-missing count — a
  thread with no validity mask reports its full `size()`.
- For a `Bool` `Series` specifically, `count_true` switches between counting
  `true`s and counting `false`s — a fundamentally different question from
  missingness.
- `count(false)` on any non-`Bool` payload throws `"Thread::count(false) is only
  valid for Bool Series payload"`.

## See Also

- [`Thread::size`](size.md): the thread's total length, missing or not.
- [`Thread::isnull`](isnull.md) / [`Thread::notnull`](notnull.md): inspect
  missingness position by position instead of a single count.
- [`Thread::any`](any.md) / [`Thread::all`](all.md): other `Bool`-only scalar
  reductions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="corr_with.html">
    <span class="label">Previous</span>
    <span class="title">corr_with</span>
  </a>
  <a class="next-link" href="covar.html">
    <span class="label">Next</span>
    <span class="title">covar</span>
  </a>
</div>
