<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 16789a5bce4c -->

# Shelf::rand

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
Shelf Shelf::rand(const Shelf& other) const
```

## What It Does

The reflected form of [`land()`](land.md). Logical AND is commutative, so `rand()`
and `land()` always compute the **same boolean values**. The display labels on the
result are a different story:

```{important} rand()'s result labels come from other, not from *this
Unlike the arithmetic family (where `radd()` reuses `add()`'s exact metadata rules)
and unlike `DataFrame::rand()` (which always carries labels from the object it was
called on, matching `land()` exactly), `Shelf::rand()` is implemented as
`land()` with the two operands swapped internally — which means the display labels
on `rand()`'s result come from **`other`**, not from `*this`. This is a real,
observable difference from `land()`, not just an API-symmetry alias.
```

## Parameters

**other** (`const Shelf&`)

Same shape as [`land()`](land.md).

## Returns

`Shelf`
    A new `Bool`-typed shelf holding the same values [`land()`](land.md) would
    produce for the same arguments — but with display labels carried from `other`
    instead of from `*this`.

## Examples

Example 1. Same values as `land()`, but the row labels come from the other side.

```cpp
using namespace pluss::table;

auto left = Shelf(pluss::make_series({true, false, true}, pluss::ScalarType::Bool))
                .label().set(Shelf(pluss::make_note({"p", "q", "r"})));
auto right = Shelf(pluss::make_series({true, true, false}, pluss::ScalarType::Bool))
                 .label().set(Shelf(pluss::make_note({"x", "y", "z"})));

auto forward = left.land(right);
auto reflected = left.rand(right);
std::cout << forward << "\n" << reflected;
```

### Output

```text
p  true
q  false
r  false

ptype: Series
x  true
y  false
z  false

ptype: Series
```

The boolean values are identical in both outputs (`true, false, false`) — only the
row labels differ: `land()` shows `left`'s labels (`p, q, r`), `rand()` shows
`right`'s (`x, y, z`), because internally `left.rand(right)` computes
`right`-AND-`left` with `right` supplying the metadata.

## Notes

- If neither shelf has display labels set, this distinction is invisible — both
  methods fall back to the same default `0, 1, 2, ...` row index either way.
- This label-carry difference is specific to `Shelf`; the equivalent
  [`DataFrame::rand`](../dataframe/rand.md) was fixed in an earlier revision to
  always carry metadata from `*this`, matching `land()` exactly. `Shelf::rand()`
  has no such fix — treat this page's behavior as the current, verified source
  truth rather than assuming symmetry with the `DataFrame` version.

## See Also

- [`Shelf::land`](land.md): the forward form, and the canonical reference for this
  family's shared mechanics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="radd.html">
    <span class="label">Previous</span>
    <span class="title">radd</span>
  </a>
  <a class="next-link" href="rank.html">
    <span class="label">Next</span>
    <span class="title">rank</span>
  </a>
</div>
