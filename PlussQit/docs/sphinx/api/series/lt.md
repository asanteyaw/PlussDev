<!-- pluss-hand-authored -->

# Series::lt

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::lt(const Series& rhs) const -> Series
Series::lt(double rhs) const -> Series
Series& Series::lt_(const Series& rhs) const
```

## What It Does

Returns an elementwise strict less-than mask.

`lt()` answers the question "is the left-hand value strictly smaller than the
right-hand value?" The word strictly matters: equality does not pass. If two
compared elements are the same, the corresponding output element is `false`.

This makes `lt()` a natural tool for open-ended thresholds and exclusive upper
bounds.

## Overload Map

`lt(rhs)`

Compares against another `Series`.

`lt(scalar)`

Compares every element in `self` against one scalar threshold.

`self < rhs` and `self < scalar`

Operator sugar for the same strict comparison.

## Parameters

**rhs** (`const Series&` or scalar)

The comparison target.

The `Series` overload accepts:

- same-shaped input
- broadcastable input
- scalar-shaped input

The scalar overload uses the same one-value threshold for every element.

## Returns

`Series`
    A new boolean `Series` with `true` wherever `self` is strictly less than
    the broadcasted right-hand side.

## Gradient

`lt()` is a boolean comparison surface, not a numeric transform. Treat its
output as a mask for downstream control flow, not as a value stream you expect
to differentiate through.

## In-Place Form

`lt_()` is the in-place sibling of [`lt()`](lt.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `lt_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
auto y = pluss::make_series({2.0, 2.0, 1.0}, {3});

x.lt_(y);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, false, false]
```

## Examples

Example 1. Scalar comparison gives a threshold mask.

```cpp
auto base = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

pluss::series::print(std::cout, base.lt(4.0));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=
⎡  true  true  true ⎤
⎣ false false false ⎦
```

Example 2. A row-shaped right-hand side broadcasts and compares column by
column.

```cpp
auto row = pluss::make_series({2.0, 2.0, 5.0});
std::cout << base.lt(row);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[true, false, true],
  [false, false, false]]
```

Example 3. The `<` operator is the same comparison surface.

```cpp
std::cout << (base < row);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[true, false, true],
  [false, false, false]]
```

Example 4. Equality does not pass a strict comparison.

```cpp
std::cout << pluss::make_series({1.0, 2.0, 3.0}).lt(2.0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, false, false]
```

Example 5. `NaN` never compares as less-than.

```cpp
auto lhs = pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), -1.0});
auto rhs = pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), 0.0});

std::cout << lhs.lt(rhs);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[false, false, true]
```

Example 6. Broadcast failures are reported directly.

```cpp
try {
  std::cout << base.lt(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}}));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
broadcast: incompatible shapes
```

## Gotchas And Failure Modes

- `lt()` is strict. Equality returns `false`.
- `NaN` does not count as less-than anything, including another `NaN`.
- The result is boolean, so think of it as a mask rather than numeric data.
- Shape rules are unchanged from the rest of the elementwise surface.

## Summary

Use `lt()` for exclusive upper bounds and open intervals. If the equality case
must count as a match, step to [`Series::le`](le.md) instead.

## Aliases

**`less()`** / **`less_()`** — spelling is kept for torch-style naming familiarity.
Each resolves to exactly the same dispatch as `lt()` / `lt_()`
before any computation runs, so behavior, gradient, and failure modes are
identical; nothing here is a separate implementation to learn.

## See Also

- [`Series::le`](le.md): inclusive less-than-or-equal.
- [`Series::gt`](gt.md): strict greater-than.
- [`Series::eq`](eq.md): exact equality.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="logsumexp.html">
    <span class="label">Previous</span>
    <span class="title">logsumexp</span>
  </a>
  <a class="next-link" href="mT.html">
    <span class="label">Next</span>
    <span class="title">mT</span>
  </a>
</div>
