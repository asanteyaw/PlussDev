# DataFrame::sort

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::sort(bool ascending = true, bool nulls_last = true) -> DataFrameSortBuilder
```

## What It Does

Creates the sort planner used for explicit value-based frame ordering.

`sort()` itself does not execute anything. It builds an ordering plan which you
finish with a terminal such as:

- `.yield()` to materialize the sorted frame
- `.order()` to return the reusable row permutation
- `.head(n)` / `.tail(n)` to take ranked slices directly

The planner grows beyond `arrange(...)` when you want:

- chained per-key policies with `.by(...).desc().then(...).asc()`
- a reusable permutation artifact from `.order()`
- within-group ordering through `.within(...).by(...)`

## Parameters

**ascending**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

**nulls_last**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

## Returns

`DataFrameSortBuilder`
    Planner surface for chained sort configuration and explicit terminals.

## Examples

Example 1. Sort rows by one or more feature values through the explicit builder chain.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sort().by({"sales", "city"}).yield();
```

### Output

```text
id  sales  city
3    90.0  Kumasi
1   100.0  Accra
2   120.0  Tema
```

Example 2. Chain per-key direction and null policy without dropping into
`ArrangeSpec`.

```cpp
auto ordered = df.sort()
    .by("sales").desc()
    .then("city").asc()
    .yield();
```

Example 3. Reuse the row permutation across aligned frames.

```cpp
auto ord = df.sort().by("sales").desc().order();
auto left = df.take(ord);
auto right = peer.take(ord);
```

Example 4. Sort within first-seen groups instead of globally sorting the group
keys themselves.

```cpp
auto within = df.sort()
    .within({"sector"})
    .by("sales").desc()
    .yield();
```

## Notes

- `sort()` is not a standalone terminal. Use `.by(...)` to define the sort keys.
- Use `arrange(...)` when you want the low-level execute-now sorter directly.
- Use `label().sort(...)` for case-axis sorting. `sort()` is for value-based row
  ordering.
- `within(...)` preserves the first-seen order of groups, then sorts rows inside
  each group by the configured sort keys.


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="sql.html">
    <span class="label">Next</span>
    <span class="title">sql</span>
  </a>
</div>
