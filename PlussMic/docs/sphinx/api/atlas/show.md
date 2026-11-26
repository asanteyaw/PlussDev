<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c92c98fe3f4d -->

# Atlas::show

[Back to Atlas API Reference](../../atlas_api_reference)

## Signatures

```cpp
void Atlas::show(const display::DisplayOptions& options = {}) const
void Atlas::show(std::ostream& os, const display::DisplayOptions& options = {}) const
```

## What It Does

The `std::ostream&` overload writes the atlas's summary — the same rendering
[`to_string()`](to_string.md) returns as a string, plus a trailing newline `to_string()`
omits. The no-argument overload opens the PlussView window: one frame at a time as a table
of its own, with a **Level** menu (First, Previous, Next, Last, Go to…) to move between
frames and the same sorting, filter, Visit, number-format and theme menus a `DataFrame`
window has. Inside a PlussII notebook it hands the atlas to the notebook as the same
summary instead of opening a window.

```{important} The no-argument overload is confirmed to hang outside an interactive terminal
`Atlas::show()` (no `std::ostream&` argument) has been directly verified to hang
indefinitely when run non-interactively. The `std::ostream&`
overload does **not** exhibit this hang — it prints immediately and returns, verified
directly. Until the no-argument overload is fixed, always call `show(std::cout, options)`
explicitly rather than `show(options)`/`show()`.
```

## Parameters

**os** (`std::ostream&`, two-argument overload)

The destination stream to write to (e.g. `std::cout`, an `std::ostringstream`). Prefer
this overload — see the important callout above.

**options** (`const display::DisplayOptions&`, optional)

Rendering options shared with the rest of the display layer.

## Returns

`void`
    Writes as a side effect; produces no return value.

## Examples

Example 1. Write explicitly to `std::cout` via the `std::ostream&` overload — the safe
form, per the important callout above.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Shelf(pluss::make_series({1.0, 2.0}))},
});
auto atlas = pluss::table::Atlas({f0}, pluss::table::Shelf(pluss::make_note({"jan"})));

atlas.show(std::cout);
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

Example 2. Capture into an `std::ostringstream`; confirm it's one byte longer than
[`to_string()`](to_string.md) (the trailing newline).

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Shelf(pluss::make_series({1.0, 2.0}))},
});
auto atlas = pluss::table::Atlas({f0});

std::ostringstream oss;
atlas.show(oss);

std::cout << "longer than to_string() by: "
          << (oss.str().size() - atlas.to_string().size()) << " byte(s)";
```

### Output

```text
longer than to_string() by: 1 byte(s)
```

## Notes

- See the important callout above before ever calling the no-argument overload — use
  `show(std::cout, options)` instead.
- One byte longer than [`to_string()`](to_string.md): the trailing newline this method
  appends.

## See Also

- [`Atlas::to_string`](to_string.md): the same rendering, returned as a string (minus the
  trailing newline) instead of written to a stream.
- [`Archive::show`](../archive/show.md): the depth-stacked-catalog analog of this method
  (single `std::ostream&` overload only, no no-argument hang).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scribbelize.html">
    <span class="label">Previous</span>
    <span class="title">scribbelize</span>
  </a>
  <a class="next-link" href="slice_frames.html">
    <span class="label">Next</span>
    <span class="title">slice_frames</span>
  </a>
</div>
