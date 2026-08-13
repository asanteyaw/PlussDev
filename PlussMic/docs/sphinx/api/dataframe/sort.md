<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8b097d0a101e -->

# DataFrame::sort

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrameSortBuilder DataFrame::sort(bool ascending = true, bool nulls_last = true) const
```

## What It Does

The entry point into the frame's reusable sort-planning workflow. `sort()`
binds the current frame — along with the *default* direction and null
placement every subsequently-added key inherits unless overridden — and
hands back a `DataFrameSortBuilder`:

```cpp
class DataFrameSortBuilder {
public:
  DataFrameSortBuilder by(...) const;      // plan the primary key(s)
  DataFrameSortBuilder then(...) const;    // append tie-break key(s)
  DataFrameSortBuilder within(...) const;  // partition into groups first
  DataFrameSortBuilder asc() const;        // mark the active key ascending
  DataFrameSortBuilder desc() const;       // mark the active key descending
  DataFrameSortBuilder nulls_first() const;
  DataFrameSortBuilder nulls_last() const;
  Thread order() const;    // materialize the row permutation
  DataFrame yield() const; // execute and return the sorted frame
  DataFrame head(std::int64_t n) const;
  DataFrame tail(std::int64_t n) const;
};
```

```{important} sort() alone plans nothing — no key is set until by()
`sort()` only records the default direction/null-placement and binds the
frame; calling a terminal (`yield()`/`order()`/`head()`/`tail()`) before any
`.by(...)` throws — see [`DataFrameSortBuilder::by`](../dataframe-sort/by.md)'s
own edge-case example.
```

```{important} Validation is lazy — unknown feature names surface at the terminal, not at by()/then()
`.by("nope")` itself never checks whether `"nope"` exists on the frame — the
unknown-feature throw only fires when a terminal method
(`yield()`/`order()`/`head()`/`tail()`) actually executes the plan. You can
chain several `.by(...)`/`.then(...)` calls referencing features that don't
exist yet without any of them throwing, right up until you call a terminal.
```

## Parameters

**ascending** (`bool`, default `true`)
    The default sort direction every key added via `.by(...)`/`.then(...)`
    inherits, unless that key overrides it (e.g. via an `ArrangeSpec`, or a
    later `.desc()` call).

**nulls_last** (`bool`, default `true`)
    The default null placement every key inherits, unless overridden.

## Returns

`DataFrameSortBuilder`
    A binder for the current frame — call `.by(...)` to start planning value
    keys, `.within(...)` to partition into groups first, then a terminal
    method to execute.

## Examples

Example 1. The simplest plan: one key, default ascending direction, executed
with `.yield()`.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city",  Thread(pluss::make_note({"Tema", "Accra", "Accra", "Tema"}))},
    {"sales", Thread(pluss::make_series({120.0, 100.0, 90.0, 200.0}))},
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

Example 2 (edge case). Pass `ascending = false` to `sort()` itself to change
the *default* every subsequently-planned key inherits, rather than calling
`.desc()` afterward.

```cpp
auto out2 = df.sort(false).by("sales").yield();
std::cout << out2;
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

## Notes

- See the important callouts above — `sort()` alone plans nothing, and
  feature-name validation is deferred to the terminal call.
- `ascending`/`nulls_last` set the *builder's* defaults; any per-key override
  (an explicit `.asc()`/`.desc()`/`.nulls_first()`/`.nulls_last()` call, or an
  `ArrangeSpec` with its own fields set) wins for that key specifically.
- Not `noexcept`; `sort()` itself does no validation work — every check is
  deferred to the terminal.

## See Also

- [DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference):
  the full method list this builder exposes.
- [`DataFrameSortBuilder::by`](../dataframe-sort/by.md): the method that
  actually starts the key plan.
- [`DataFrame::arrange`](arrange.md): the direct execute-now sorter, for when
  you don't need a reusable plan.
- [`DataFrame::take`](take.md): the row-selection primitive
  [`order()`](../dataframe-sort/order.md)'s materialized permutation feeds into.

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
