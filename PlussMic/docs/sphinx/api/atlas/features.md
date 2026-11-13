<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 54543aaa7c0f -->

# Atlas::features

[Back to Atlas API Reference](../../atlas_api_reference)

## Signatures

```cpp
Atlas Atlas::features(std::string_view feature) const
Atlas Atlas::features(std::initializer_list<std::string_view> features) const
Atlas Atlas::features(pluss::msharp::RangeView::Of<std::string_view> features) const
```

## What It Does

Returns a new atlas keeping only the named feature(s), applied to every frame — delegates
to each frame's own `DataFrame::features()`, so a name that doesn't exist throws that
frame's own `"DataFrame::features: unknown column '<name>'"`. Depth and depth labels are
preserved.

## Parameters

**feature** (`std::string_view`, single-feature overload)
    A single feature name to select.

**features** (`std::initializer_list<std::string_view>` or
`pluss::msharp::RangeView::Of<std::string_view>`, multi-feature overloads)
    Several feature names to select at once.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the receiver, with every frame reduced to
    just the named feature(s).

## Examples

Example 1. Select a single feature from every frame.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({1.0, 2.0}))},
    {"score", pluss::table::Slate(pluss::make_series({9.5, 8.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({3.0, 4.0}))},
    {"score", pluss::table::Slate(pluss::make_series({7.5, 6.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1}, pluss::table::Slate(pluss::make_note({"jan", "feb"})));

auto out = atlas.features("id");
std::cout << out.dloc(0);
```

### Output

```text
===========
     | id  
-----------
0    | 1   
1    | 2   
-----------
[2 cs x 1 feat]
```

Example 2. Select several features with an inline list.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({1.0, 2.0}))},
    {"city", pluss::table::Slate(pluss::make_note({"Accra", "Tema"}))},
    {"score", pluss::table::Slate(pluss::make_series({9.5, 8.0}))},
});
auto atlas = pluss::table::Atlas({f0});

auto out = atlas.features({"id", "score"});
std::cout << out.dloc(0);
```

### Output

```text
==================
     | id    score
------------------
0    | 1     9.5  
1    | 2     8    
------------------
[2 cs x 2 feat]
```

Example 3. Selecting a feature missing from a frame throws.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({1.0}))},
});
auto atlas = pluss::table::Atlas({f0});

try {
  atlas.features("nope");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
DataFrame::features: unknown column 'nope'
```

## Notes

- Throws on any frame missing a requested feature — every frame must have it, since
  `Atlas` requires the same schema across all frames.
- Depth and depth labels are preserved on the result.

## See Also

- [`Atlas::map`](map.md): apply a callback to every cell across every feature, instead of
  selecting a subset.
- [`DataFrame::features`](../dataframe/features.md): the single-frame delegate this method
  applies to every frame.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="flow.html">
    <span class="label">Next</span>
    <span class="title">flow</span>
  </a>
</div>
