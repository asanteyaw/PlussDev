<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1ce95b95829f -->

# Shelf::size

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
std::int64_t Shelf::size() const
```

## What It Does

Returns the number of elements (rows) in the shelf — the one dimension a `Shelf`
has, regardless of payload kind. This is the same count [`shape()`](shape.md) reports
as its first element and [`ncases()`](ncases.md) returns under a different name.

## Parameters

`None`

## Returns

`std::int64_t`
    The element count. `0` for an empty (but still defined) shelf.

## Examples

Example 1. A shelf's size is independent of payload kind.

```cpp
using namespace pluss::table;

auto numbers = Shelf(pluss::make_series({10.0, 20.0, 30.0}));
auto words = Shelf(pluss::make_note({"a", "b"}));
std::cout << numbers.size() << "\n";
std::cout << words.size();
```

### Output

```text
3
2
```

Example 2. `size()` counts every element, including `NULL` ones — it's not a
non-null count.

```cpp
auto with_nulls = Shelf(pluss::make_series({1.0, 2.0, 3.0}),
    pluss::make_series(std::vector<bool>{true, false, true}));
std::cout << with_nulls.size();
```

### Output

```text
3
```

## Notes

- Counts every element, valid or `NULL` — see [`count()`](count.md) for a
  non-null-only count.
- Calling `size()` on an undefined shelf throws `"Shelf: undefined handle"` — see
  [`defined()`](defined.md).

## See Also

- [`Shelf::ncases`](ncases.md): the same count, under DataFrame-consistent naming.
- [`Shelf::shape`](shape.md): `{size(), 1}` — the shelf's shape as a pair.
- [`Shelf::count`](count.md): the non-null element count.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shuffle.html">
    <span class="label">Previous</span>
    <span class="title">shuffle</span>
  </a>
  <a class="next-link" href="skew.html">
    <span class="label">Next</span>
    <span class="title">skew</span>
  </a>
</div>
