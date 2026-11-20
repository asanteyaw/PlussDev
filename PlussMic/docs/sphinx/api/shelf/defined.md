<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3685d9f2e68b -->

# Shelf::defined

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
bool Shelf::defined() const noexcept
```

## What It Does

Reports whether the `Shelf` object currently owns a usable backing payload at all —
an object-level integrity check, not a data-level one. A default-constructed
`Shelf()` is undefined: it has no `Series`, `Note`, `Timeline`, or `TimeIndex`
payload behind it, and calling almost any other method on it throws. This is a
different question from "does this shelf have missing values" — a fully defined
shelf can still hold `NULL` cells; see [`isnull()`](isnull.md) for that question.

```{important} Almost every other method requires defined() == true
Calling `size()`, `kind()`, `dtype()`, or any transform/reduction on an undefined
shelf throws `"Shelf: undefined handle"`. `defined()` itself is one of the very
few methods safe to call unconditionally — it never throws, regardless of whether
the shelf is defined.
```

## Parameters

`None`

## Returns

`bool`
    `true` when the shelf owns a backing payload and can participate in ordinary
    operations; `false` for a default-constructed or moved-from `Shelf`.

## Examples

Example 1. A shelf built from real data is defined.

```cpp
using namespace pluss::table;

auto scores = Shelf(pluss::make_series({10.0, 20.0, 30.0}));
auto out = scores.defined();
std::cout << std::boolalpha << out;
```

### Output

```text
true
```

Example 2. A default-constructed shelf is not defined.

```cpp
auto empty_handle = Shelf();
auto out2 = empty_handle.defined();
std::cout << std::boolalpha << out2;
```

### Output

```text
false
```

Example 3. Calling another method on an undefined shelf throws.

```cpp
try {
  auto bad = empty_handle.size();
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
Shelf: undefined handle
```

## Notes

- `defined()` never throws, even on an undefined shelf — it's the safe way to check
  before calling anything else.
- Object-level validity only: says nothing about whether individual cells are
  `NULL`. Use [`isnull()`](isnull.md) / [`notnull()`](notnull.md) for that.
- A moved-from `Shelf` (via `std::move`) is also left undefined.

## See Also

- [`Shelf::isnull`](isnull.md) / [`Shelf::notnull`](notnull.md): per-cell validity,
  as opposed to this method's whole-object validity.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cumsum.html">
    <span class="label">Previous</span>
    <span class="title">cumsum</span>
  </a>
  <a class="next-link" href="describe.html">
    <span class="label">Next</span>
    <span class="title">describe</span>
  </a>
</div>
