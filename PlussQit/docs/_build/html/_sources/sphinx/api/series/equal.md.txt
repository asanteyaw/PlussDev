<!-- pluss-hand-authored -->

# Series::equal

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
bool Series::equal(const Series& other) const
```

## What It Does

Returns one C++ `bool`: `true` only when `self` and `other` have **exactly
the same shape** and **every corresponding value compares equal**.

It is the whole-tensor, exact-match decision — the right tool for an
assertion, a guard clause, or a container-level identity check where you want
a single yes/no, not a per-element mask.

It differs from [`eq`](eq.md) in two ways that matter:

- **No broadcasting.** `eq` stretches `[3]` against `[1, 3]` and compares
  elementwise; `equal` treats any shape mismatch as an immediate `false`.
- **One answer, not a mask.** `eq` returns a `Bool` `Series`; `equal`
  reduces that down to a single `bool`.

The comparison is on **values**, so operands may have different numeric
dtypes and still be `equal` if the numbers line up (`int` `1` equals `double`
`1.0`). If dtype identity is also part of your requirement, check
`dtype().scalar` separately.

There is **no tolerance** — `equal` is bit-exact-for-integers,
value-exact-for-floats. For an approximate decision use
[`allclose`](allclose.md); for the per-position approximate mask use
[`isclose`](isclose.md).

## Parameters

**other** (`const Series&`): the `Series` to compare against. Must be on the
same platform as `self` **when the shapes match** (see Gotchas).

## Returns

`bool`
    `true` iff shapes are identical and `all(self == other)`.

## Raises

- if either `Series` is undefined
- if the shapes match but the platforms differ

(A plain shape mismatch is **not** an error — it returns `false`.)

## Gradient

Not applicable — `equal()` returns a plain `bool`.

## Examples

Example 1. Exact match versus a single differing value.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0});
std::cout << std::boolalpha
          << "identical: " << a.equal(pluss::make_series({1.0, 2.0, 3.0})) << "\n"
          << "one off:   " << a.equal(pluss::make_series({1.0, 2.0, 3.0000001})) << "\n";
```

### Output

```text
identical: true
one off:   false
```

A difference of `1e-7` is still a difference — `equal` has no tolerance. If
that should count as a match, you want [`allclose`](allclose.md).

Example 2. Different dtype, equal values → `true`. `equal` compares numbers,
not representations.

```cpp
auto ints = pluss::make_series({1, 2, 3});
std::cout << "int vs float: " << std::boolalpha
          << ints.equal(pluss::make_series({1.0, 2.0, 3.0})) << "\n";
```

### Output

```text
int vs float: true
```

Example 3. A shape mismatch is `false` immediately — no broadcasting, no
error.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0});
std::cout << std::boolalpha
          << "[3] vs [1,3]: " << a.equal(a.unsqueeze(0)) << "\n"
          << "[3] vs [2]:   " << a.equal(pluss::make_series({1.0, 2.0})) << "\n";
```

### Output

```text
[3] vs [1,3]: false
[3] vs [2]:   false
```

`[3]` and `[1, 3]` are broadcast-compatible and `eq` would compare them, but
they are not the *same size*, so `equal` stops at `false`.

Example 4. `NaN` never equals `NaN` — so `equal` of a tensor with a `NaN`
against an identical copy is `false`.

```cpp
auto a = pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), 3.0});
std::cout << std::boolalpha
          << "equal(identical-with-NaN):      "
          << a.equal(pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), 3.0})) << "\n"
          << "allclose(..., equal_nan=true):  "
          << a.allclose(pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), 3.0}),
                        1e-5, 1e-8, true) << "\n";
```

### Output

```text
equal(identical-with-NaN):      false
allclose(..., equal_nan=true):  true
```

`equal` has no `equal_nan` option. When matching missing-value positions
should count, use [`allclose`](allclose.md)`(..., equal_nan = true)`.

Example 5. Two equal-shaped empty `Series` are equal — there is no position
at which they differ.

```cpp
auto e1 = pluss::series::zeros({0}, pluss::ScalarType::Float64);
auto e2 = pluss::series::zeros({0}, pluss::ScalarType::Float64);
std::cout << "empty [0] vs empty [0]: " << std::boolalpha << e1.equal(e2) << "\n";
```

### Output

```text
empty [0] vs empty [0]: true
```

Example 6. `equal` (one `bool`) versus [`eq`](eq.md) (a per-element mask).

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0});
auto b = pluss::make_series({1.0, 9.0, 3.0});
std::cout << "a.equal(b) = " << std::boolalpha << a.equal(b) << "\n";
std::cout << "a.eq(b)    = ";
pluss::series::print(std::cout, a.eq(b));
```

### Output

```text
a.equal(b) = false
a.eq(b)    = Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, false, true]
```

`equal` tells you *whether* the tensors match; `eq` tells you *where* they
diverge.

## Gotchas And Failure Modes

- **No tolerance.** A `1e-12` float difference makes `equal` `false`. Use
  [`allclose`](allclose.md) for an approximate decision.
- **No broadcasting.** Different shapes are `false`, never stretched.
- **`NaN != NaN`.** A tensor containing `NaN` is never `equal` to any
  tensor, including a byte-identical copy.
- Equal values do **not** imply equal dtype, layout, or shared storage — use
  `dtype().scalar` and [`is_set_to`](is_set_to.md) for those.
- A shape mismatch returns `false`, but a *platform* mismatch on matching
  shapes **raises** — the platform check happens only after the shape check
  passes.
- An undefined operand raises.

## Summary

`equal(other)` is the exact, whole-tensor comparison:

- `true` iff identical shape **and** every value equal — no tolerance, no
  broadcasting
- compares values, so differing numeric dtypes can still be `equal`
- `NaN` breaks it; equal-shaped empties satisfy it vacuously
- use [`eq`](eq.md) for the per-element mask, [`allclose`](allclose.md) /
  [`isclose`](isclose.md) for approximate versions

## Choosing A Comparison

| You want | Method | Broadcasts | Tolerance |
|---|---|---|---|
| One exact yes/no | `equal` | no | none |
| A per-element exact mask | [`eq`](eq.md) | yes | none |
| One approximate yes/no | [`allclose`](allclose.md) | yes | `rtol` / `atol` |
| A per-element approximate mask | [`isclose`](isclose.md) | yes | `rtol` / `atol` |

## See Also

- [`Series::eq`](eq.md): the broadcasting, per-element exact comparison.
- [`Series::allclose`](allclose.md) / [`Series::isclose`](isclose.md): the
  tolerant versions.
- [`Series::is_same_size`](is_same_size.md): the shape half of `equal`, on
  its own.
- [`Series::is_set_to`](is_set_to.md): shared-storage and layout identity —
  a stronger notion than value equality.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eq.html">
    <span class="label">Previous</span>
    <span class="title">eq</span>
  </a>
  <a class="next-link" href="erf.html">
    <span class="label">Next</span>
    <span class="title">erf</span>
  </a>
</div>
