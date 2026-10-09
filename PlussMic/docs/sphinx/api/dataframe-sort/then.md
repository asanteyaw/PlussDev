<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 27f1c27b2a6b -->

# DataFrameSortBuilder::then

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signatures

```cpp
DataFrameSortBuilder DataFrameSortBuilder::then(std::string_view feature) const
DataFrameSortBuilder DataFrameSortBuilder::then(std::initializer_list<std::string_view> features) const
DataFrameSortBuilder DataFrameSortBuilder::then(pluss::msharp::RangeView::Of<std::string_view> features) const
DataFrameSortBuilder DataFrameSortBuilder::then(pluss::msharp::RangeView::Of<DataFrame::ArrangeSpec> specs) const
```

## What It Does

Appends additional tie-break key(s) after an existing [`by(...)`](by.md)
plan — the mirror image of `by()`, which replaces the plan instead of
extending it.

The appended key is not a separate second sort pass; it becomes the next slot
in the same lexicographic / tuple ordering. So `.by("city").then("sales")`
means "sort by `(city, sales)`", with `sales` used only inside equal-`city`
ties.

```{important} then() requires an existing by() plan
Calling `.then(...)` before any `.by(...)` throws — there's no plan yet to
append to. See Example 3.
```

## Parameters

**feature** / **features** / **specs**
    Same shapes and meaning as [`by(...)`](by.md)'s corresponding overloads
    — plain names inherit the builder's direction/null-placement defaults;
    `ArrangeSpec`s carry their own.

## Returns

`DataFrameSortBuilder`
    A new builder with the extra key(s) appended after the existing plan.
    The last appended key becomes the active key.

## Examples

Example 1. Sort by `"city"` primarily, breaking ties within each city by
`"sales"` descending.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city",  Slate(pluss::make_note({"Tema", "Accra", "Accra", "Tema"}))},
    {"sales", Slate(pluss::make_series({120.0, 100.0, 90.0, 200.0}))},
});

auto out = df.sort().by("city").then("sales").desc().yield();
std::cout << out;
```

### Output

```text
===================
     | city   sales
-------------------
1    | Accra  100  
2    | Accra  90   
3    | Tema   200  
0    | Tema   120  
-------------------
[4 cs x 2 feat]
```

Within `"Accra"`, `100` sorts before `90` (descending); within `"Tema"`,
`200` sorts before `120`. `.desc()` after `.then("sales")` modifies only the
`"sales"` key just appended — `"city"` stays ascending.

Example 2 (edge case). Calling `.then(...)` with no prior `.by(...)` throws.

```cpp
try {
  auto out2 = df.sort().then("sales").yield();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
DataFrame::sort.then: no existing sort plan; call .by(...) first
```

Example 3. `ArrangeSpec`s let appended tie-break keys carry their own direction.

```cpp
auto out3 = df.sort()
              .by("city")
              .then({DataFrame::ArrangeSpec{"sales", false, true}})
              .yield();
std::cout << out3;
```

### Output

```text
===================
     | city   sales
-------------------
1    | Accra  100  
2    | Accra  90   
3    | Tema   200  
0    | Tema   120  
-------------------
[4 cs x 2 feat]
```

The primary key remains `city` ascending. The appended `sales` key only breaks
ties inside each city, and it does so descending because its `ArrangeSpec`
overrides the builder default.

## Notes

- See the important callout above.
- Like `by()`, feature-name validation on the appended key(s) is deferred to
  the terminal method, not `.then(...)` itself.
- The last-appended key becomes the active key for
  [`asc()`](asc.md)/[`desc()`](desc.md)/[`nulls_first()`](nulls_first.md)/
  [`nulls_last()`](nulls_last.md).
- Plain feature-name forms inherit one shared builder default; use
  `ArrangeSpec` when an appended tie-break key needs its own direction.

## See Also

- [`DataFrameSortBuilder::by`](by.md): starts (and replaces) the plan this
  method appends to.
- [`DataFrameSortBuilder::desc`](desc.md): used above to override the
  appended key's direction.
- [`DataFrame::sort`](../dataframe/sort.md): the entry point this builder is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="within.html">
    <span class="label">Next</span>
    <span class="title">within</span>
  </a>
</div>
