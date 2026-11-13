<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: cfd673d5fda8 -->

# Slate::matmul

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
SlateValue Slate::matmul(const Slate& other) const
```

## What It Does

Computes the dot product of two numeric slates — `sum(this[i] * other[i])` over every
row — and returns it wrapped in a `SlateValue` rather than a full `Slate`. Both operands
must be numeric `Series`-kind slates; every other payload kind is rejected.

```{important} matmul() tolerates no NULL values at all, including ones introduced by alignment
Unlike most arithmetic on `Slate` (which quietly skips or propagates `NULL` rows),
`matmul()` refuses to run if *either* operand has any `NULL` at all — whether those `NULL`s
were already present, or introduced because the two slates don't line up one-to-one (for
example, mismatched lengths with no shared case labels to align by). Either situation
produces the same throw. See Examples 3 and 4.
```

## Parameters

**other** (`const Slate&`)
    The right-hand numeric slate to dot with `*this`. Must be numeric `Series`-kind, with
    no `NULL` values, and must line up one-to-one with `*this` (matching length, or
    matching case labels if both slates carry them).

## Returns

`SlateValue`
    A single-value wrapper holding the dot-product result. Extract it with `.to_dbl()` (or
    the other `SlateValue` accessors) rather than treating it like a multi-row `Slate`.

## Examples

Example 1. Dot product of two equal-length numeric slates — read the scalar out with
`.to_dbl()`.

```cpp
using namespace pluss::table;

auto a = Slate(pluss::make_series({10.0, 20.0, 30.0}));
auto b = Slate(pluss::make_series({15.0, 10.0, 35.0}));

auto out = a.matmul(b);
std::cout << out.to_dbl();
```

### Output

```text
1400
```

`10*15 + 20*10 + 30*35 = 150 + 200 + 1050 = 1400`.

Example 2. `SlateValue`'s own `operator<<` prints the raw underlying `Series` debug
representation, not the friendly row-indexed rendering `Slate` uses — prefer `.to_dbl()`
(as in Example 1) when you just want the number.

```cpp
auto a = Slate(pluss::make_series({10.0, 20.0, 30.0}));
auto b = Slate(pluss::make_series({15.0, 10.0, 35.0}));

auto out = a.matmul(b);
std::cout << out;
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1400]
```

Example 3. Calling `matmul()` on a non-numeric slate throws.

```cpp
auto notes = Slate(pluss::make_note({"a", "b"}));
auto numbers = Slate(pluss::make_series({1.0, 2.0}));
try {
  auto out = notes.matmul(numbers);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::matmul: expected a real numeric Series Slate
```

Example 4. Mismatched lengths (with no shared case labels to align by) also throw — the
message is the same `NULL`-rejection message as an explicit `NULL`, since the two slates
can't be lined up one-to-one without introducing a gap.

```cpp
auto a = Slate(pluss::make_series({1.0, 2.0}));
auto b = Slate(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto out = a.matmul(b);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::matmul: null values are not supported
```

## Notes

- Requires both operands to be numeric `Series`-kind; throws otherwise.
- No tolerance for `NULL` in either operand — including `NULL`s introduced implicitly by
  a failed one-to-one alignment. See the important callout above.
- Returns `SlateValue`, not `Slate` — extract the scalar with `.to_dbl()` rather than
  printing the `SlateValue` directly (Example 2 shows why).

## See Also

- [`Slate::sum`](sum.md): the reduction `matmul()`'s dot product is built from
  conceptually (`(a * b).sum()`), for when you don't need the elementwise product retained.
- [`Slate::mul`](mul.md): elementwise multiplication, if you want the per-row products
  instead of their sum.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matches.html">
    <span class="label">Previous</span>
    <span class="title">matches</span>
  </a>
  <a class="next-link" href="max.html">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>
