<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 594aed4d5e9a -->

# DataFrame::put_

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame& DataFrame::put_(std::string_view feature, const Slate& bool_mask, const Slate& value)
DataFrame& DataFrame::put_(std::string_view feature, const Slate& bool_mask, const Slate& value, const AlignOptions& options)
DataFrame& DataFrame::put_(std::string_view feature, const Slate& bool_mask, double value)
DataFrame& DataFrame::put_(std::string_view feature, const Slate& bool_mask, double value, const AlignOptions& options)
DataFrame& DataFrame::put_(std::string_view feature, const Slate& bool_mask, std::string_view value)
DataFrame& DataFrame::put_(std::string_view feature, const Slate& bool_mask, std::string_view value, const AlignOptions& options)
DataFrame& DataFrame::put_(std::string_view feature, const Slate& bool_mask, const char* value)
DataFrame& DataFrame::put_(std::string_view feature, const Slate& bool_mask, const char* value, const AlignOptions& options)
DataFrame& DataFrame::put_(std::string_view feature, const Slate& bool_mask, pluss::null_t value)
DataFrame& DataFrame::put_(std::string_view feature, const Slate& bool_mask, pluss::null_t value, const AlignOptions& options)
// each Slate-mask overload above also has a pluss::Series-mask convenience:
DataFrame& DataFrame::put_(std::string_view feature, const pluss::Series& bool_mask, /* value as above */)
```

## What It Does

The in-place counterpart to [`put()`](put.md): rewrites one feature's values at the
rows where `bool_mask` is `true`, mutating `*this` directly and returning a reference
to it, instead of returning a new frame. Every parameter and replacement rule is
identical to `put()` — see that page for the full breakdown of scalar-vs-`Slate`
replacement semantics; this page only covers what differs about mutating in place.

## Parameters

**feature** (`std::string_view`)

The feature to rewrite in place. Must already exist on the frame.

**bool_mask** (`const Slate&` or `const pluss::Series&`)

A boolean row mask, identical in behavior to [`put()`](put.md)'s `bool_mask`.

**value** (`const Slate&`, `double`, `std::string_view`, `const char*`, or
`pluss::null_t`)

The replacement, identical in behavior to [`put()`](put.md)'s `value` — a scalar
broadcasts to every masked row, a `Slate` supplies a different value per row.

**options** (`const AlignOptions&`, optional)

Controls how `bool_mask` is aligned to the frame's case axis, identical to
[`put()`](put.md)'s `options`.

## Returns

`DataFrame&`
    A reference to `*this`, after `feature`'s masked rows have been rewritten.

## Examples

Example 1. Rewrite a feature's masked rows in place, then keep working with the same
frame.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Slate(pluss::make_series({1, 2, 3}))},
    {"sales", Slate(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto is_middle = orders.features("id") == 2.0;
orders.put_("sales", is_middle, 999.0);
std::cout << orders;
```

### Output

```text
==========================
     | id    sales  city  
--------------------------
0    | 1     100    Accra 
1    | 2     999    Tema  
2    | 3     90     Kumasi
--------------------------
[3 cs x 3 feat]
```

`orders` itself now has `999` at case `1` — no separate result variable was needed,
unlike [`put()`](put.md), which would have left `orders` unchanged and returned a new
frame instead.

Example 2. Chain a second in-place rewrite onto the same frame.

```cpp
orders.put_("city", is_middle, "Tamale");
std::cout << orders;
```

### Output

```text
============================
     | id    sales  city    
----------------------------
0    | 1     100    Accra   
1    | 2     999    Tamale  
2    | 3     90     Kumasi  
----------------------------
[3 cs x 3 feat]
```

Both rewrites accumulate on the same `orders` object — case `1`'s `sales` (from
Example 1) and now its `city` too, both replaced in place.

## Notes

- `put_()` mutates and returns `*this` by reference; it never allocates a new
  `DataFrame` the way `put()` does.
- Any other `DataFrame` value that shares the same underlying columns (via a prior
  copy) does not see this mutation — `pluss::table::DataFrame`'s copies are
  value-like, not aliased references to shared state.
- See [`put()`](put.md) for the complete parameter and replacement-semantics
  breakdown; every rule there (scalar vs. per-row `Slate` replacement, `Series`-mask
  convenience overloads, kind constraints) applies identically here.

## See Also

- [`DataFrame::put`](put.md): the non-mutating counterpart, returning a new frame
  instead of mutating in place.
- [`DataFrame::mask`](mask.md): nulls out masked cells outright, rather than
  replacing them with a chosen value.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="put.html">
    <span class="label">Previous</span>
    <span class="title">put</span>
  </a>
  <a class="next-link" href="query.html">
    <span class="label">Next</span>
    <span class="title">query</span>
  </a>
</div>
