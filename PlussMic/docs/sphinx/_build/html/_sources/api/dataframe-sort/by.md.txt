<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4b209a5977c9 -->

# DataFrameSortBuilder::by

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signatures

```cpp
DataFrameSortBuilder DataFrameSortBuilder::by(std::string_view feature) const
DataFrameSortBuilder DataFrameSortBuilder::by(std::initializer_list<std::string_view> features) const
DataFrameSortBuilder DataFrameSortBuilder::by(pluss::msharp::RangeView::Of<std::string_view> features) const
DataFrameSortBuilder DataFrameSortBuilder::by(pluss::msharp::RangeView::Of<DataFrame::ArrangeSpec> specs) const
```

## What It Does

Starts (or replaces) the sort plan's primary key list. When you pass plain
feature names, each one inherits the builder's current defaults (set on
[`DataFrame::sort(ascending, nulls_last)`](../dataframe/sort.md)) for
direction and null placement. When you pass `DataFrame::ArrangeSpec`s
directly, each spec carries its own `ascending`/`nulls_last`, overriding the
builder defaults for that key specifically.

With more than one key, `by(...)` establishes a lexicographic / tuple key:
the first entry is primary, the next entry only breaks ties within equal
earlier-key values, and so on.

```{important} by() replaces the plan; it does not append
Calling `.by(...)` a second time on the same builder discards whatever keys
were set by an earlier `.by(...)` call — it doesn't add to them. Use
[`then(...)`](then.md) to append additional tie-break keys to an existing
plan instead.
```

```{important} Unknown feature names don't throw until a terminal method runs
`.by("nope")` doesn't validate `"nope"` against the frame — the throw only
happens when [`yield()`](yield.md)/[`order()`](order.md)/[`head()`](head.md)/
[`tail()`](tail.md) actually executes the plan. See Example 4.
```

## Parameters

**feature** (`std::string_view`)
    A single feature name to sort by.

**features** (`std::initializer_list<std::string_view>` or
`pluss::msharp::RangeView::Of<std::string_view>`)
    Several feature names to sort by, in the order given; each inherits the
    builder's current direction/null-placement defaults.

**specs** (`pluss::msharp::RangeView::Of<DataFrame::ArrangeSpec>`)
    Several `{feature, ascending, nulls_last}` specs, each carrying its own
    direction and null placement independent of the builder defaults.

## Returns

`DataFrameSortBuilder`
    A new builder with the primary key plan set to exactly the keys given,
    replacing any prior plan. The last key becomes the *active* key for a
    subsequent [`asc()`](asc.md)/[`desc()`](desc.md)/
    [`nulls_first()`](nulls_first.md)/[`nulls_last()`](nulls_last.md) call.

## Examples

Example 1. A single primary key, default ascending.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city",  Slate(pluss::make_note({"Tema", "Accra", "Accra", "Tema"}))},
    {"sales", Slate(pluss::make_series({120.0, 100.0, 90.0, 200.0}))},
});

auto out = df.sort().by("sales").yield();
std::cout << out;
```

### Output

```text
===================
     | city   sales
-------------------
2    | Accra  90   
1    | Accra  100  
0    | Tema   120  
3    | Tema   200  
-------------------
[4 cs x 2 feat]
```

Example 2. Several feature names at once, ordered as tie-break keys —
`"city"` first (primary), then `"sales"` (tie-break).

```cpp
auto out2 = df.sort().by({"city", "sales"}).yield();
std::cout << out2;
```

### Output

```text
===================
     | city   sales
-------------------
2    | Accra  90   
1    | Accra  100  
0    | Tema   120  
3    | Tema   200  
-------------------
[4 cs x 2 feat]
```

Example 3. `ArrangeSpec`s let each key carry its own direction directly,
without a follow-up [`desc()`](desc.md) call.

```cpp
DataFrame::ArrangeSpec spec{"sales", /*ascending=*/false, /*nulls_last=*/true};
auto out3 = df.sort().by({spec}).yield();
std::cout << out3;
```

### Output

```text
===================
     | city   sales
-------------------
3    | Tema   200  
0    | Tema   120  
1    | Accra  100  
2    | Accra  90   
-------------------
[4 cs x 2 feat]
```

Example 4. Two `ArrangeSpec`s give independent directions per key.

```cpp
auto out4 = df.sort().by({
    DataFrame::ArrangeSpec{"city", true, true},
    DataFrame::ArrangeSpec{"sales", false, true},
}).yield();
std::cout << out4;
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

This is the mixed-direction case plain feature-name overloads cannot express:
`city` is ascending, while `sales` breaks ties descending inside each city.

Example 5 (edge case). An unknown feature name doesn't throw at `.by(...)`
time — only when a terminal method executes the plan.

```cpp
try {
  auto out5 = df.sort().by("nope").yield();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
DataFrame::sort.order: unknown feature 'nope'
```

## Notes

- See the important callouts above — `by()` replaces rather than appends,
  and validation is deferred to the terminal.
- The last-supplied key becomes the active key for the direction/null-
  placement mutators.
- When you pass only feature names, all of them inherit one shared direction
  and null-placement default from the builder. Per-key control requires
  `ArrangeSpec`.
- Feature names must exist on the frame — checked lazily, per Example 4.

## See Also

- [`DataFrameSortBuilder::then`](then.md): append tie-break keys instead of
  replacing the plan.
- [`DataFrameSortBuilder::within`](within.md): partition into groups before
  applying this key plan inside each group.
- [`DataFrame::sort`](../dataframe/sort.md): the entry point this builder is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="asc.html">
    <span class="label">Previous</span>
    <span class="title">asc</span>
  </a>
  <a class="next-link" href="desc.html">
    <span class="label">Next</span>
    <span class="title">desc</span>
  </a>
</div>
