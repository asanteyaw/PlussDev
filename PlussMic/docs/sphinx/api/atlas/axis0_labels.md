<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5c7c12ce7704 -->

# Atlas::axis0_labels

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::axis0_labels() const -> const std::optional<Thread>&
```

## What It Does

Returns the labels attached to the depth axis — the name given to each stacked frame's
position, the same role a `DataFrame`'s case labels play for rows. Unlike case labels,
depth labels are **optional**: an `Atlas` constructed without an explicit label `Thread`
has `axis0_labels()` return an empty `std::optional`, and every depth-label-aware method
(`loc()`, [`sort_depth()`](sort_depth.md), [`align_depth()`](align_depth.md)) either
requires labels to be set first or falls back to plain integer positions. This is the raw
accessor; [`label()`](label.md) wraps it with a friendlier get/set/sort/align surface.

## Parameters

`None`

## Returns

`const std::optional<Thread>&`
    The depth-axis labels, if any were set; an empty `std::optional` otherwise.

## Examples

Example 1. An atlas constructed with explicit depth labels.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"sales", Thread(pluss::make_series({100.0, 120.0}))}});
auto q2 = DataFrame({{"sales", Thread(pluss::make_series({110.0, 90.0}))}});
auto atlas = Atlas({q1, q2}, Thread(pluss::make_note({"2024Q1", "2024Q2"})));

std::cout << atlas.axis0_labels().has_value() << "\n";
std::cout << *atlas.axis0_labels();
```

### Output

```text
1
0    2024Q1
1    2024Q2
```

Example 2. An atlas constructed without labels has none — `axis0_labels()` reports an
empty `std::optional`, not a default integer range.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"sales", Thread(pluss::make_series({100.0, 120.0}))}});
auto q2 = DataFrame({{"sales", Thread(pluss::make_series({110.0, 90.0}))}});
auto atlas = Atlas({q1, q2});

std::cout << atlas.axis0_labels().has_value();
```

### Output

```text
0
```

## Notes

- Requires a defined `Atlas`; throws `"Atlas: undefined handle"` on a
  default-constructed `Atlas()`.
- The absence of labels is a distinct state from labels being `0, 1, 2, ...` — dereference
  the returned `std::optional` only after checking `has_value()`, or use
  [`label().values()`](label.md), which fills in default integer labels automatically
  when none are set.
- When present, depth labels must have exactly `depth()` entries — enforced at
  construction time.

## See Also

- [`Atlas::label`](label.md): the friendlier accessor built on top of this raw optional
  — `values()` defaults to integer labels instead of throwing or requiring a manual
  `has_value()` check.
- [`Atlas::relabel_depth`](relabel_depth.md): replace the depth labels with a new
  `Thread`.
- [`Atlas::sort_depth`](sort_depth.md) / [`Atlas::align_depth`](align_depth.md): reorder
  the stack using these labels.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="align_depth.html">
    <span class="label">Previous</span>
    <span class="title">align_depth</span>
  </a>
  <a class="next-link" href="column_names.html">
    <span class="label">Next</span>
    <span class="title">column_names</span>
  </a>
</div>
