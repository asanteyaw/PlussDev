<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0292317d45e7 -->

# AtlasLabelAccessor::has

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signature

```cpp
bool AtlasLabelAccessor::has() const
```

## What It Does

Reports whether the bound atlas has depth labels set at all — the same check as
[`Atlas::label().has()`](../../atlas_api_reference).

## Parameters

`None`

## Returns

`bool`
    `true` if the atlas has depth labels set; `false` otherwise.

## Examples

Example 1. An unlabeled atlas reports `false`; setting labels flips it to `true`.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({1.0}))},
});
auto atlas = pluss::table::Atlas({f0});

std::cout << "has() before: " << atlas.label().has() << "\n";

auto labeled = atlas.label().set(pluss::table::Slate(pluss::make_note({"jan"})));
std::cout << "has() after: " << labeled.label().has();
```

### Output

```text
has() before: 0
has() after: 1
```

## Notes

- Never throws, regardless of whether depth labels are set.
- Every other accessor method that reads the depth labels ([`values()`](values.md), and
  the set-algebra/lookup helpers) requires `has()` to be `true` first — check it before
  calling them on a possibly-unlabeled atlas.

## See Also

- [`AtlasLabelAccessor::values`](values.md): read the depth labels this method confirms
  exist.
- [`AtlasLabelAccessor::set`](set.md) / [`AtlasLabelAccessor::reset`](reset.md): set or
  clear the depth labels this method checks for.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="find.html">
    <span class="label">Previous</span>
    <span class="title">find</span>
  </a>
  <a class="next-link" href="intersect.html">
    <span class="label">Next</span>
    <span class="title">intersect</span>
  </a>
</div>
