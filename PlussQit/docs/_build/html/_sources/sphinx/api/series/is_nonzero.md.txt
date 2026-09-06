<!-- pluss-hand-authored -->

# Series::is_nonzero

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
bool Series::is_nonzero() const
```

## What It Does

Collapses a **single-element** `Series` to one `bool`: `true` if that one
value is not equal to `0`, `false` if it is.

"Single element" means `numel() == 1` — a rank-`0` scalar and a `[1]` (or
`[1, 1]`, …) vector both qualify. Anything with more than one value, and
every empty `Series`, **raises** rather than guessing what you meant.

That strictness is the point. `is_nonzero()` is the safe way to turn a
computed condition into a C++ `bool` for an `if`, precisely because it
refuses a multi-value tensor instead of silently testing just the first
element or the whole thing. When you *do* mean "are all values nonzero" or
"is any value nonzero", say so with [`all()`](all.md) or [`any()`](any.md)
and then call `is_nonzero()` on that one-element result.

The comparison is exactly `value != 0`:

- `NaN != 0` is `true`, so `is_nonzero()` of `NaN` is `true`.
- `-0.0 == 0.0`, so `is_nonzero()` of `-0.0` is `false`.
- a `Bool` `true` is `true`; `Bool` `false` is `false`.

## Parameters

None.

## Returns

`bool`
    `true` iff the sole element compares `!= 0`.

## Raises

- if `numel() != 1` (too many elements, or an empty `Series`)
- if the `Series` is undefined

## Gradient

Not applicable — `is_nonzero()` returns a plain `bool`.

## Examples

Example 1. Zero is `false`; any other value is `true`. A `[1]` vector works
just like a scalar.

```cpp
std::cout << std::boolalpha
          << "make_series(0.0):   " << pluss::make_series(0.0).is_nonzero() << "\n"
          << "make_series(3.0):   " << pluss::make_series(3.0).is_nonzero() << "\n"
          << "make_series({5.0}): " << pluss::make_series({5.0}).is_nonzero() << "\n";
```

### Output

```text
make_series(0.0):   false
make_series(3.0):   true
make_series({5.0}): true
```

Example 2. The comparison is literally `!= 0` — with the IEEE consequences.

```cpp
std::cout << std::boolalpha
          << "NaN:   " << pluss::make_series(std::nan("")).is_nonzero() << "\n"
          << "-0.0:  " << pluss::make_series(-0.0).is_nonzero() << "\n"
          << "true:  " << pluss::make_series(true).is_nonzero() << "\n"
          << "false: " << pluss::make_series(false).is_nonzero() << "\n";
```

### Output

```text
NaN:   true
-0.0:  false
true:  true
false: false
```

`NaN` is "nonzero" because it is not equal to `0` (it is not equal to
anything). If you mean "is this a real, present number", test
[`isnan`](isnan.md) first.

Example 3. More than one element raises — it does not test just the first.

```cpp
try {
  pluss::make_series({1.0, 2.0}).is_nonzero();
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
is_nonzero: expected a Series with exactly one element
```

Example 4. An empty `Series` raises too — `numel() == 0` is not one element.

```cpp
try {
  pluss::series::zeros({0}).is_nonzero();
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
is_nonzero: expected a Series with exactly one element
```

Example 5. The intended pattern for a multi-value condition: reduce
explicitly, then collapse.

```cpp
auto pred = pluss::make_series({0.2, 0.9, 0.5});
auto all_above = pred.gt(0.1).all();       // one-element Bool Series
auto any_above = pred.gt(0.8).any();
std::cout << std::boolalpha
          << "all > 0.1? " << all_above.is_nonzero() << "\n"
          << "any > 0.8? " << any_above.is_nonzero() << "\n";
```

### Output

```text
all > 0.1? true
any > 0.8? true
```

`all()` / `any()` do the reduction you actually mean; `is_nonzero()` just
turns their one-element result into a branchable `bool`.

## Gotchas And Failure Modes

- The **exactly one element** rule is by rank *and* size: a `[1]` vector
  passes, a `[2]` vector and every empty `Series` raise.
- `NaN` counts as nonzero (`NaN != 0`). Filter with [`isnan`](isnan.md)
  first if that is not what you want.
- `-0.0` counts as zero.
- Never use it as a stand-in for [`all()`](all.md) / [`any()`](any.md) — it
  cannot reduce, it can only collapse a result that is already a single
  value.

## Summary

`is_nonzero()` turns a one-element `Series` into a `bool`:

- requires `numel() == 1` (scalar or `[1]`); anything else, and every empty
  `Series`, raises
- the test is `value != 0` — `NaN` → `true`, `-0.0` → `false`
- pair with [`all()`](all.md) / [`any()`](any.md) to branch on a
  multi-value condition

## See Also

- [`Series::all`](all.md) / [`Series::any`](any.md): the reductions to run
  *before* `is_nonzero()` when the condition has more than one value.
- [`Series::numel`](numel.md): the count `is_nonzero()` insists is `1`.
- [`Series::isnan`](isnan.md): distinguish a real value from the `NaN` that
  `is_nonzero()` reports as nonzero.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_mps.html">
    <span class="label">Previous</span>
    <span class="title">is_mps</span>
  </a>
  <a class="next-link" href="is_prime.html">
    <span class="label">Next</span>
    <span class="title">is_prime</span>
  </a>
</div>
