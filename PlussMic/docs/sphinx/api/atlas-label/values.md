<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4d8f2d518021 -->

# AtlasLabelAccessor::values

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signature

```cpp
AtlasLabelAccessor::values() const -> Slate
```

## What It Does

Returns the atlas's depth-axis labels as a `Slate` — the same values
[`Atlas::axis0_labels()`](../atlas/axis0_labels.md) exposes, but always as a concrete
`Slate` rather than a `std::optional<Slate>`. If the atlas was constructed without
explicit depth labels, `values()` materializes the default `0, 1, 2, ...` integer
positions on the fly instead of requiring a `has_value()` check first — the same
"always defined" convenience [`DataFrame::label().values()`](../dataframe-label/values.md)
offers for the case axis, applied here to the depth axis instead.

## Returns

`Slate`
    The depth labels: whatever `Slate` was set at construction (or via
    [`set()`](set.md)/[`range()`](range.md)), or a default `0, 1, 2, ...` integer
    sequence if none was ever set.

## Examples

Example 1. An atlas constructed with explicit depth labels.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"sales", Slate(pluss::make_series({100.0, 120.0}))}});
auto q2 = DataFrame({{"sales", Slate(pluss::make_series({110.0, 90.0}))}});
auto atlas = Atlas({q1, q2}, Slate(pluss::make_note({"2024Q1", "2024Q2"})));

auto out = atlas.label().values();
std::cout << out;
```

### Output

```text
0    2024Q1
1    2024Q2
ptype: Note
```

Example 2. An atlas constructed *without* depth labels still returns a concrete
`Slate` from `values()` — the default integer positions, not an empty result.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"sales", Slate(pluss::make_series({100.0, 120.0}))}});
auto q2 = DataFrame({{"sales", Slate(pluss::make_series({110.0, 90.0}))}});
auto atlas = Atlas({q1, q2});

std::cout << "axis0_labels has_value: " << atlas.axis0_labels().has_value() << "\n";
std::cout << atlas.label().values();
```

### Output

```text
axis0_labels has_value: 0
0    0
1    1
ptype: Series
```

`atlas.axis0_labels()` reports no value at all here, but `atlas.label().values()` never
leaves you checking for that — it fills in the default integer sequence itself.

## Notes

- Always returns a concrete `Slate`; never throws for lack of labels the way a raw
  `*atlas.axis0_labels()` dereference would on an atlas with none set.
- The default labels are exactly `0` through `depth() - 1`, matching the row numbers
  `Atlas::frame(i)`/`Atlas::dloc(i)` already use to index the stack.

## See Also

- [`Atlas::axis0_labels`](../atlas/axis0_labels.md): the raw, optional form this method
  wraps.
- [`AtlasLabelAccessor::has`](has.md): check whether real labels were set, without
  materializing the default sequence.
- [`AtlasLabelAccessor::set`](set.md): replace the depth labels with a new `Slate`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unite.html">
    <span class="label">Previous</span>
    <span class="title">unite</span>
  </a>
</div>
