<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9044ef2fd600 -->

# Shelf::value

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
ShelfValue Shelf::value() const
```

## What It Does

Wraps `*this` in a `ShelfValue` — a lightweight box around a whole `Shelf` that
adds typed scalar-extraction helpers (`to_dbl()`, `to_int()`, `to_str()`, `host()`)
and implicit conversions to the underlying payload types (`pluss::Series`,
`pluss::Note`, `pluss::Timeline`, `pluss::TimeIndex`). `value()` itself does **not**
reduce or require a single-element shelf — it wraps the shelf as-is, at whatever
length it already has. The reduction methods ([`sum()`](sum.md), [`mean()`](mean.md),
[`min()`](min.md), and the rest) also return `ShelfValue`, typically already
length-1 after their own reduction; `value()` is the direct way to get the same box
type around a shelf you already have, without any reduction step.

```{important} Scalar accessors require exactly one element
`to_dbl()`, `to_int()`, `to_str()`, and `host()` all check that the wrapped shelf has
`size() == 1`, throwing `"ShelfValue::host: Shelf payload must contain exactly one
value"` otherwise. Calling `.value()` on a multi-element shelf is legal; calling
`.to_dbl()` on the result of that isn't. See Example 3.
```

## Parameters

`None`

## Returns

`ShelfValue`
    A box around `*this`, same length and kind as the source shelf.

## Examples

Example 1. Wrap a single-element shelf and pull out its scalar value.

```cpp
using namespace pluss::table;

auto single = Shelf(pluss::make_series({42.0}));
auto boxed = single.value();
std::cout << boxed.to_dbl();
```

### Output

```text
42
```

Example 2. `value()` works on `Note`-kind shelves too, via `to_str()`.

```cpp
auto label = Shelf(pluss::make_note({"Accra"}));
std::cout << label.value().to_str();
```

### Output

```text
Accra
```

Example 3. `value()` on a multi-element shelf succeeds — the scalar accessors are
what enforce the single-element requirement, not `value()` itself.

```cpp
auto many = Shelf(pluss::make_series({1.0, 2.0, 3.0}));
auto boxed_many = many.value();
std::cout << std::boolalpha << boxed_many.defined() << " " << static_cast<int>(boxed_many.kind()) << "\n";

try {
  auto bad = boxed_many.to_dbl();
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
true 0
ShelfValue::host: Shelf payload must contain exactly one value
```

## Notes

- `value()` never reduces or validates length — it's a plain wrapper.
- `to_dbl()` / `to_int()` / `to_str()` / `host()` all require the wrapped shelf to
  have exactly one element.
- `ShelfValue` also converts implicitly back to `pluss::Series` / `pluss::Note` /
  `pluss::Timeline` / `pluss::TimeIndex`, matching the wrapped shelf's `kind()`.

## See Also

- [`Shelf::sum`](sum.md) / [`Shelf::mean`](mean.md): reductions that return a
  `ShelfValue` already narrowed to a single element.
- [`Shelf::kind`](kind.md): the payload kind a `ShelfValue`'s scalar accessor
  dispatches on internally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="upper.html">
    <span class="label">Previous</span>
    <span class="title">upper</span>
  </a>
  <a class="next-link" href="var.html">
    <span class="label">Next</span>
    <span class="title">var</span>
  </a>
</div>
