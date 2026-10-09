<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 71c86fb526e2 -->

# Atlas::loc

[Back to Atlas API Reference](../../atlas_api_reference)

## Signatures

```cpp
Atlas::loc(std::string_view label) const -> DataFrame
Atlas::loc(std::int64_t label) const -> DataFrame
Atlas::loc(const pluss::Timeline& label) const -> DataFrame
```

## What It Does

Fetches the frame whose **depth label** matches `label` — the label-based counterpart to
[`dloc()`](dloc.md)/[`frame()`](frame.md)'s positional access, and the `Atlas` analog of
[`Archive::loc`](../archive/loc.md). The overload is chosen by `label`'s C++ type, and must
match the depth labels' own payload kind: `std::string_view` for `Note`-kind depth labels,
`std::int64_t` for numeric (`Series`) depth labels, `pluss::Timeline` for `Timeline`-kind
depth labels.

`loc(std::int64_t)` requires the depth labels to actually be `Series`-kind (numeric); it
throws immediately — `"Atlas::loc(int64): axis0 labels must be numeric Series"` — if they
are `Note`, `Timeline`, or `TimeIndex` instead, rather than attempting to coerce them. This
matches the corrected behavior of [`Archive::loc(std::int64_t)`](../archive/loc.md) — it
fails loudly on a kind mismatch instead of guessing.

## Parameters

**label** (`std::string_view` / `std::int64_t` / `const pluss::Timeline&`)
    The depth label to search for. Its C++ type must match the depth labels' own payload
    kind; `loc(std::int64_t)` throws immediately if the depth labels aren't `Series`-kind.

## Returns

`DataFrame`
    The frame whose depth label equals `label`.

## Examples

Example 1. `loc(std::string_view)` on a `Note`-kind (text) depth-labeled atlas.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({1.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({2.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1}, pluss::table::Slate(pluss::make_note({"train", "test"})));

std::cout << atlas.loc("test");
```

### Output

```text
===========
     | id  
-----------
0    | 2   
-----------
[1 cs x 1 feat]
```

Example 2. `loc(std::int64_t)` on a genuinely numeric-labeled atlas works correctly.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({1.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({2.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1}, pluss::table::Slate(pluss::make_series({100.0, 200.0})));

std::cout << atlas.loc(std::int64_t(200));
```

### Output

```text
===========
     | id  
-----------
0    | 2   
-----------
[1 cs x 1 feat]
```

Example 3. Calling `loc(std::int64_t)` on a `Note`-labeled (text) atlas throws immediately.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({1.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({2.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1}, pluss::table::Slate(pluss::make_note({"train", "test"})));

try {
  atlas.loc(std::int64_t(0));
} catch (const std::exception& e) {
  std::cout << "threw: " << e.what();
}
```

### Output

```text
threw: Atlas::loc(int64): axis0 labels must be numeric Series
```

Example 4. `loc(const pluss::Timeline&)` on a `Timeline`-kind depth-labeled atlas.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({1.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({2.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1},
    pluss::table::Slate(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-02-01T00:00:00Z"})));

auto key = pluss::make_timeline({"2024-02-01T00:00:00Z"});
std::cout << atlas.loc(key);
```

### Output

```text
===========
     | id  
-----------
0    | 2   
-----------
[1 cs x 1 feat]
```

## Notes

- `loc(std::int64_t)` throws `"Atlas::loc(int64): axis0 labels must be numeric Series"` if
  the depth labels aren't `Series`-kind — check with
  [`axis0_labels()`](axis0_labels.md)/[`label().values()`](label.md)'s `kind()` first if
  you're unsure.
- A genuinely unmatched label (correct kind, but no such value) throws
  `"Atlas::loc(<type>): label not found"`.

## See Also

- [`Atlas::dloc`](dloc.md) / [`Atlas::frame`](frame.md): positional access by depth-index
  instead of label.
- [`Atlas::label`](label.md): inspect, set, or check the kind of the depth labels this
  method searches.
- [`Archive::loc`](../archive/loc.md): the depth-stacked-catalog analog of this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="label.html">
    <span class="label">Previous</span>
    <span class="title">label</span>
  </a>
  <a class="next-link" href="map.html">
    <span class="label">Next</span>
    <span class="title">map</span>
  </a>
</div>
