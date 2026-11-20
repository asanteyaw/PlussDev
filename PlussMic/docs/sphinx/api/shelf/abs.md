<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: cb57d8d99419 -->

# Shelf::abs

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
Shelf Shelf::abs() const
```

## What It Does

Returns the elementwise absolute value of a `Series`-kind shelf — strips the sign,
leaving magnitude. Requires the shelf to be `Series`-kind.

## Parameters

`None`

## Returns

`Shelf`
    A new `Series`-kind shelf, same length as `*this`, holding each element's
    absolute value.

## Examples

Example 1. Absolute value strips sign; zero and positive values are unchanged.

```cpp
using namespace pluss::table;

auto deltas = Shelf(pluss::make_series({-2.5, 0.0, 3.5}));
auto out = deltas.abs();
std::cout << out;
```

### Output

```text
0  2.5
1  0
2  3.5

ptype: Series
```

Example 2. A non-`Series` shelf throws.

```cpp
auto notes = Shelf(pluss::make_note({"a", "b"}));
try {
  auto bad = notes.abs();
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
Shelf::abs is only valid for Series payload
```

## Notes

- Requires the calling shelf to be `Series`-kind.
- A `NULL` cell on input stays `NULL` on output.

## See Also

- [`Shelf::clip`](clip.md): cap values into a range, rather than stripping sign.
- [`Shelf::pow`](pow.md): more general elementwise numeric transform.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="add.html">
    <span class="label">Next</span>
    <span class="title">add</span>
  </a>
</div>
