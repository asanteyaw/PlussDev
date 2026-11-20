<!-- pluss-hand-authored -->

# ShelfLabelAccessor::reset

[Back to ShelfLabelAccessor API Reference](../../shelf_label_accessor_api_reference)

## Signature

```cpp
Shelf ShelfLabelAccessor::reset() const
```

## What It Does

Clears the bound shelf's case labels entirely — a direct forward to
`Shelf::clear_caselbl()`. Unlike [`range()`](range.md), which *attaches* an
explicit default-range labels shelf, `reset()` removes the case-label
attachment altogether, so the shelf displays with the default `0, 1, 2, ...`
positions computed on the fly rather than stored.

## Parameters

`None`

## Returns

`Shelf`
    A new shelf, holding the same values as `*this`, with no case labels
    attached.

## Examples

Example 1. Clear custom labels, and confirm `has()` now reports `false`.

```cpp
using namespace pluss::table;

auto tagged = Shelf(pluss::make_series({100.0, 200.0, 150.0}))
                  .label().set(Shelf(pluss::make_note({"a", "b", "c"})));
std::cout << tagged;

auto plain = tagged.label().reset();
std::cout << plain;
std::cout << plain.label().has();
```

### Output (before)

```text
a  100
b  200
c  150
ptype: Series
```

### Output (after reset())

```text
0  100
1  200
2  150
ptype: Series
```

### Output (has())

```text
0
```

The display looks the same as [`range()`](range.md)'s result, but `has()`
reports `false` here — `reset()` removed the case-label attachment entirely
rather than attaching a fresh default range.

## Notes

- Produces the same visible `0, 1, 2, ...` display as [`range()`](range.md),
  but [`has()`](has.md) reports `false` afterward.
- Identical to calling `Shelf::clear_caselbl()` directly.

## See Also

- [`ShelfLabelAccessor::range`](range.md): visually identical result, but
  actively attaches a default-range labels shelf rather than clearing one.
- [`ShelfLabelAccessor::set`](set.md): the operation `reset()` undoes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="repeated.html">
    <span class="label">Previous</span>
    <span class="title">repeated</span>
  </a>
  <a class="next-link" href="set.html">
    <span class="label">Next</span>
    <span class="title">set</span>
  </a>
</div>
