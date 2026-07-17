# DataFrame::feataxis

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::feataxis() -> DataFrameFeatureAxisAccessor
```

Return the feature-axis helper surface.

`feataxis()` is the compact surface for schema-driven feature selection and feature-name transforms. It is deliberately smaller than the main `DataFrame` API:

- `df.features()` returns feature names.
- `df.features("price")` selects one feature as a `Thread`.
- `df.features({"price", "volume"})` selects explicit features as a `DataFrame`.
- `df.feataxis().like("price")` selects features whose names contain a substring.
- `df.feataxis().match("^score_")` selects features whose names match a regex pattern.
- `df.feataxis().series()`, `note()`, and `timeline()` select features by payload kind.
- `df.feataxis().prefix(...)` and `suffix(...)` rewrite feature names in bulk.

The surface intentionally does not expose `names()`, `has(...)`, `ref(...)`, or `set(...)`. Use `features()` for names, `get(name).has_value()` for public feature-existence checks, and `rename(...)` for full positional feature-name replacement.

## Parameters

`None`

## Returns

`DataFrameFeatureAxisAccessor`
    Lightweight feature-axis accessor bound to the current frame.

    Selector and renaming methods on this accessor return a new `DataFrame`; the source frame is not mutated.

## Notes

Use `like(...)` for simple substring matching. Use `match(...)` when you need regex anchors, alternation, character classes, or other pattern logic.

`feataxis()` filters or rewrites the feature axis. It does not inspect text values inside `Note` payloads; use the `note()` surface for text-value regex work.

Full positional replacement of all feature names is done with `df.rename({"new_a", "new_b", ...})`; the feature-axis helper surface intentionally has no setter-style method.

## Examples

Example 1. Select features by substring.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"score_math", pluss::make_series({80.0, 91.0})},
    {"score_final", pluss::make_series({88.0, 93.0})},
    {"name", pluss::make_note({"Ada", "Bo"})},
});

auto out = df.feataxis().like("score");
```

### Output

```text
score_math  score_final
80          88
91          93
```

Example 2. Select features by regex.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"A1", pluss::make_series({1.0, 2.0})},
    {"A2", pluss::make_series({3.0, 4.0})},
    {"AA", pluss::make_series({5.0, 6.0})},
});

auto out = df.feataxis().match("^A[0-9]+$");
```

### Output

```text
A1  A2
1   3
2   4
```

Example 3. Select features by payload kind.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema"})},
    {"sales", pluss::make_series({10.0, 12.0})},
});

auto numeric = df.feataxis().series();
auto text = df.feataxis().note();
```

### Output

```text
numeric:
sales
10
12

text:
city
Accra
Tema
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="factor.html">
    <span class="label">Previous</span>
    <span class="title">factor</span>
  </a>
  <a class="next-link" href="features.html">
    <span class="label">Next</span>
    <span class="title">features</span>
  </a>
</div>
