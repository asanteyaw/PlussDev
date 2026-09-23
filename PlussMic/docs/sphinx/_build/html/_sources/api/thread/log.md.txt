<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6237a6d89079 -->

# Thread::log

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::log() const -> Thread
```

## What It Does

Applies the elementwise natural logarithm (`ln`). Requires the thread to be
`Series`-kind. A non-floating scalar type (integer, bool) is promoted to `Float64`
before the logarithm is computed — `log()` never throws on integer input purely for
being integer.

Standard IEEE-754 domain behavior applies: `log(0.0)` produces `-Inf`, and
`log(negative)` produces `NaN`, rather than throwing.

## Parameters

`None`

## Returns

`Thread`
    A new `Series`-kind thread, same length as `*this`, holding the natural log of
    each element (promoted to a floating scalar type if the input was integral).

## Examples

Example 1. Natural log of positive values.

```cpp
using namespace pluss::table;

auto values = Thread(pluss::make_series({1.0, 10.0, 100.0}));
auto out = values.log();
std::cout << out;
```

### Output

```text
0  0
1  2.302585093
2  4.605170186

ptype: Series
```

Example 2. `log(0.0)` and `log(negative)` don't throw — they produce `-Inf` and
`NaN`, following standard IEEE-754 domain conventions.

```cpp
auto edge_cases = Thread(pluss::make_series({0.0, -5.0}));
auto out2 = edge_cases.log();
std::cout << out2;
```

### Output

```text
0  -inf
1  nan

ptype: Series
```

Example 3. A non-`Series` thread throws.

```cpp
auto notes = Thread(pluss::make_note({"a", "b"}));
try {
  auto bad = notes.log();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::log is only valid for Series payload
```

## Notes

- Requires the calling thread to be `Series`-kind.
- Integer/bool input is promoted to `Float64` automatically before computing.
- `log(0.0)` yields `-Inf`; `log(negative)` yields `NaN` — neither throws. Check
  with [`isfinite()`](isfinite.md) / [`isnan()`](isnan.md) afterward if that matters.
- A `NULL` cell on input stays `NULL` on output.

## See Also

- [`Thread::exp`](exp.md): the inverse transform.
- [`Thread::pow`](pow.md): general exponentiation, for bases other than *e*.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="length.html">
    <span class="label">Previous</span>
    <span class="title">length</span>
  </a>
  <a class="next-link" href="lor.html">
    <span class="label">Next</span>
    <span class="title">lor</span>
  </a>
</div>
