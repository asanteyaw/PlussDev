<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d0e3581a397f -->

# Shelf::exp

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
Shelf Shelf::exp() const
```

## What It Does

Applies the elementwise natural exponential (`e^x`) — the inverse of
[`log()`](log.md). Requires the shelf to be `Series`-kind. A non-floating scalar
type (integer, bool) is promoted to `Float64` before the exponential is computed.

## Parameters

`None`

## Returns

`Shelf`
    A new `Series`-kind shelf, same length as `*this`, holding `exp(x)` for each
    element (promoted to a floating scalar type if the input was integral).

## Examples

Example 1. Exponentiate a small numeric shelf.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({0.0, 1.0, 2.0}));
auto out = values.exp();
std::cout << out;
```

### Output

```text
0  1
1  2.718281828
2  7.389056099

ptype: Series
```

Example 2. `log()` and `exp()` round-trip back to the original values (subject to
ordinary floating-point precision).

```cpp
auto original = Shelf(pluss::make_series({5.0, 12.5, 100.0}));
auto round_trip = original.log().exp();
std::cout << round_trip;
```

### Output

```text
0  5
1  12.5
2  100

ptype: Series
```

Example 3. A non-`Series` shelf throws.

```cpp
auto notes = Shelf(pluss::make_note({"a", "b"}));
try {
  auto bad = notes.exp();
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
Shelf::exp is only valid for Series payload
```

## Notes

- Requires the calling shelf to be `Series`-kind.
- Integer/bool input is promoted to `Float64` automatically before computing.
- A `NULL` cell on input stays `NULL` on output.

## See Also

- [`Shelf::log`](log.md): the inverse transform.
- [`Shelf::pow`](pow.md): general exponentiation, for bases other than *e*.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eq.html">
    <span class="label">Previous</span>
    <span class="title">eq</span>
  </a>
  <a class="next-link" href="extract.html">
    <span class="label">Next</span>
    <span class="title">extract</span>
  </a>
</div>
