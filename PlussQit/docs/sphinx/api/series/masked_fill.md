<!-- pluss-hand-authored -->

# Series::masked_fill

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::masked_fill(const Series& mask, double value) const
Series Series::masked_fill(const Series& mask, const Series& value) const
Series& Series::masked_fill_(const Series& mask, double value) const
Series& Series::masked_fill_(const Series& mask, const Series& value) const
Series& Series::masked_fill_(const Series& mask, double value) const
Series& Series::masked_fill_(const Series& mask, const Series& value) const
```

## What It Does

Replaces every position where a boolean mask is `true` with one scalar,
leaving everything else exactly as it was:

```text
result[i] = mask[i] ? value : self[i]
```

This is [`index_fill`](index_fill.md)'s free-form sibling: `index_fill`
selects whole slices along one axis by integer position, `masked_fill`
selects an *arbitrary* elementwise pattern by boolean condition. Whenever the
positions to overwrite come from a comparison (`x.gt(0)`, `x.isnan()`, a
padding mask) rather than a fixed list of row/column indices, `masked_fill`
is the tool — and since it is built on [`where`](where.md), it stays fully
vectorized and differentiable no matter how irregular the mask's pattern is.

`mask` broadcasts against `self` in the out-of-place form, so a `[1, N]` mask
row can stamp the same pattern onto every row of an `[M, N]` receiver, or a
`[1, N]` receiver can be broadcast out to match an `[M, N]` mask. Two
overloads cover `value`:

- **`double value`** — an ordinary constant with no gradient of its own.
- **`const Series& value`** — a **rank-zero** (`shape=[]`) scalar. Use this
  form when the fill value is itself something you are optimizing and needs
  to receive a gradient.

## Parameters

**mask** (`const Series&`): a `Bool` series, broadcast-compatible with
`self`. Sharing `self`'s platform is required.

**value** (`double`, or a rank-zero `const Series&`): the replacement. A
`Series` value must have `shape=[]` — a length-one vector (`shape=[1]`) is
not a scalar for this contract and is rejected. Either form is converted to
`self`'s dtype (by truncation for integral dtypes, not rounding).

## Returns

`Series`
    The broadcast shape of `self` and `mask`. `self` is untouched.
    `masked_fill_` mutates `self` in place and returns a reference to it —
    but only when `mask` already broadcasts *to* `self`'s existing shape; it
    can never grow the receiver.

## Gradient

Differentiable in `self` and, for the `Series`-valued overload, in `value`.
Sensitivity passes straight through at every `false` position and is zero at
every `true` position — the receiver's original value there no longer
influences the result. When `value` is itself a tracked `Series`, every
`true` position's upstream sensitivity is summed back into that one scalar.
Broadcasting is honored in reverse the usual way: if `self` expands from
`[1, N]` to `[M, N]`, its sensitivity sums the contributions from all `M`
broadcast rows. `mask` itself is non-differentiable.

## In-Place Form

`masked_fill_()` is the in-place sibling of [`masked_fill()`](masked_fill.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `masked_fill_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);

x.masked_fill_(mask, 0.0);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 2, 0]
```

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto value = pluss::make_series(0.0);  // must be rank-zero

x.masked_fill_(mask, value);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 2, 0]
```

## Examples

Example 1. The simplest case: mask and receiver already share a shape, no
broadcasting involved.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
auto mask = pluss::make_series({false, true, false, true, false});
pluss::series::print(std::cout, values.masked_fill(mask, -1.0));
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, -1.0000, 3.0000, -1.0000, 5.0000]
```

Positions `1` and `3` (where `mask` is `true`) become `-1`; the rest are
untouched.

Example 2. Both operands can broadcast: a one-row receiver against a
two-row mask.

```cpp
auto values = pluss::make_series({{1.0, 2.0, 3.0}});
auto mask = pluss::make_series({{true, false, true},
                                {false, true, false}});
pluss::series::print(std::cout, values.masked_fill(mask, 9.0));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 9.0000 2.0000 9.0000 ⎤
⎣ 1.0000 9.0000 3.0000 ⎦
```

The single receiver row is broadcast to two rows before the mask is applied;
`true` positions in either row become `9`, `false` positions read the
(broadcast) receiver value.

Example 3. A rank-zero `Series` fill value participates in reverse mode.

```cpp
auto values = pluss::make_series({{1.0, 2.0, 3.0}});
auto fill = pluss::make_series(9.0);
auto mask = pluss::make_series({{true, false, true},
                                {false, true, false}});
values.tracks_slope_(true);
fill.tracks_slope_(true);
values.masked_fill(mask, fill).sum().reverse();
std::cout << "dvalues = "; pluss::series::print(std::cout, values.sensitivity());
std::cout << "dfill   = "; pluss::series::print(std::cout, fill.sensitivity());
```

### Output

```text
dvalues = Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=
[ 1.0000 1.0000 1.0000 ]
dfill   = Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=3.0000
```

Each column of the one-row receiver survives at least once across the two
broadcast rows, so every column gets sensitivity `1`. `fill` is used at three
`true` positions and receives the sum of all three, `3`.

Example 4. In-place masking can never grow the receiver.

```cpp
auto values = pluss::make_series({{1.0, 2.0, 3.0}});
auto mask = pluss::make_series({{true, false, true},
                                {false, true, false}});
try {
  values.masked_fill_(mask, 9.0);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
masked_fill_: shape mismatch (in-place requires mask broadcast to match a)
```

The out-of-place call in Example 2 was free to produce a new `[2, 3]`
result; the in-place call cannot turn the existing `[1, 3]` receiver into a
different shape.

Example 5. When the mask already matches the receiver's shape, the in-place
form succeeds and mutates directly.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0});
auto mask = pluss::make_series({true, false, true});
values.masked_fill_(mask, 0.0);
pluss::series::print(std::cout, values);
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 2.0000, 0.0000]
```

Example 6. The two extremes: an all-`false` mask changes nothing, an
all-`true` mask replaces everything.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0});
pluss::series::print(std::cout, values.masked_fill(pluss::make_series({false, false, false}), 9.0));
std::cout << "\n";
pluss::series::print(std::cout, values.masked_fill(pluss::make_series({true, true, true}), 9.0));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0000, 2.0000, 3.0000]

Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[9.0000, 9.0000, 9.0000]
```

Example 7. `value` is converted to the receiver's dtype by truncation, not
rounding, for an integral receiver.

```cpp
auto values = pluss::make_series({1, 2, 3});
auto mask = pluss::make_series({true, false, true});
pluss::series::print(std::cout, values.masked_fill(mask, 9.7));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[9, 2, 9]
```

`9.7` becomes `9`, not `10` — the same truncating conversion
[`to`](to.md) uses everywhere else.

Example 8. A non-boolean mask is rejected outright — build the condition
explicitly first.

```cpp
try {
  auto values = pluss::make_series({1.0, 2.0});
  auto mask = pluss::make_series({1.0, 0.0});   // numeric, not Bool
  auto y = values.masked_fill(mask, -1.0);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
masked_fill: mask must be bool
```

Reach for a comparison (`values.gt(0.5)`) or [`to`](to.md)`(Bool)` to
produce one.

## Gotchas And Failure Modes

- The mask must have dtype `Bool`. Numeric masks (even `0`/`1`) are rejected
  — compare or cast first.
- A `Series` fill value must have `shape=[]` exactly; `shape=[1]` is a
  one-element vector, not a scalar, and is rejected.
- Out-of-place mask broadcasting can enlarge the result; the in-place form
  can never enlarge the receiver — the mask must already broadcast *to*
  `self`'s existing shape.
- `value` converts to `self`'s dtype by truncation for integral receivers,
  not rounding.
- Use the `Series`-valued overload only when `value` needs a stored gradient;
  otherwise the plain `double` form is simpler and carries no sensitivity
  bookkeeping.

## Summary

`masked_fill(mask, value)` is boolean selection with one replacement scalar:

- `true` positions become `value`; `false` positions keep `self`'s value
- `mask` broadcasts against `self` out-of-place; in-place it may not enlarge
  the receiver
- `value` is a plain `double` or a rank-zero `Series` (for a differentiable
  fill)
- differentiable: zero sensitivity at `true` positions, pass-through at
  `false` ones, summed into a `Series` fill value

## See Also

- [`Series::where`](where.md): select between two full `Series` values
  instead of one constant.
- [`Series::masked_select`](masked_select.md): extract the `true` positions
  into a flat 1-D result instead of overwriting them in place.
- [`Series::index_fill`](index_fill.md): fill whole slices selected by
  integer position along one axis, rather than an arbitrary boolean pattern.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mT.html">
    <span class="label">Previous</span>
    <span class="title">mT</span>
  </a>
  <a class="next-link" href="masked_scatter.html">
    <span class="label">Next</span>
    <span class="title">masked_scatter</span>
  </a>
</div>
