<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 14cfaba3a439 -->

# Atlas::to_string

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
std::string Atlas::to_string(const display::DisplayOptions& options = {}) const
```

## What It Does

Renders the atlas's summary as a `std::string`: a short list of facts that stays the same
length however many frames the atlas stacks or however many columns they have — how deep it
is, the range of cases and features per frame, whether the feature names are the same in
every frame, a tally of the column kinds, and the depth labels. Same rendering `operator<<`
and [`show(std::ostream&)`](show.md) write to a stream, minus the trailing newline those two
append.

## Parameters

**options** (`const display::DisplayOptions&`, optional)

Rendering options shared with the rest of the display layer.

## Returns

`std::string`
    The rendered atlas summary, without a trailing newline.

## Examples

Example 1. Render an atlas to a string.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Shelf(pluss::make_series({1.0, 2.0}))},
});
auto atlas = pluss::table::Atlas({f0}, pluss::table::Shelf(pluss::make_note({"jan"})));

std::cout << atlas.to_string();
```

### Output

```text
========================================================================================================================
Atlas
------------------------------------------------------------------------------------------------------------------------
fact          | value
------------------------------------------------------------------------------------------------------------------------
depth         | 1 frames
cases         | 2 each
features      | 1 each
feature names | the same in every frame
ptypes        | Series × 1
depth labels  | Note: jan to jan
========================================================================================================================
```

Example 2. `to_string()` is one byte shorter than the equivalent
[`show(std::ostream&)`](show.md) capture — the missing trailing newline, same pattern as
[`Archive::to_string`](../archive/to_string.md).

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Shelf(pluss::make_series({1.0, 2.0}))},
});
auto atlas = pluss::table::Atlas({f0}, pluss::table::Shelf(pluss::make_note({"jan"})));

auto str = atlas.to_string();
std::ostringstream oss;
atlas.show(oss);

std::cout << "to_string length: " << str.size() << "\n";
std::cout << "show length: " << oss.str().size();
```

### Output

```text
to_string length: 683
show length: 684
```

## Notes

- This is a summary, not the frames: it never lists feature names or cases. To see a frame's
  own table, print `atlas.frame(i)`; to browse them all, open the window with
  [`show()`](show.md), which shows one frame at a time.
- One byte shorter than [`show(std::ostream&)`](show.md)/`operator<<`'s output — see
  Example 2.

## See Also

- [`Atlas::show`](show.md): the same rendering, written to a stream instead of returned as
  a string — see that page for an important callout about the no-argument overload.
- [`Archive::to_string`](../archive/to_string.md): the depth-stacked-catalog analog of this
  method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tl.html">
    <span class="label">Previous</span>
    <span class="title">tl</span>
  </a>
  <a class="next-link" href="ts.html">
    <span class="label">Next</span>
    <span class="title">ts</span>
  </a>
</div>
