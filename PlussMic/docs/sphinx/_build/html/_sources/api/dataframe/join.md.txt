<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7ef0c5c1472c -->

# DataFrame::join

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::join(const DataFrame& other, std::string collision_suffix = "_right") const -> DataFrame
```

## What It Does

Attaches every feature from `other` onto a copy of the current frame, case-by-case, and
returns the combined frame. The two frames are aligned by their **case axis position**
— case `i` of `other` is paired with case `i` of the current frame — not by matching the
values of a shared key column.

This is the most important thing to know before reaching for `join()`: it is a
**positional** attach, appropriate for two frames that already line up case-for-case —
conceptually a horizontal concatenation of features, not a relational join. If you need
to combine two frames by matching values in a key column (an `order_id`, a `customer_id`,
and so on) rather than by position, look at [`DataFrame::sql()`](sql.md) /
[`prep_sql()`](prep_sql.md) for a key-based join expressed as SQL, since `join()` itself
performs no key matching or row reordering at all.

Both frames must have a compatible number of cases for the result to make sense — `join`
does not reindex, sort, or fill in gaps between mismatched case axes.

## Parameters

**other** (`const DataFrame&`)

The frame whose features are attached to the current frame. Every feature on `other` is
appended, in `other`'s existing feature order, after the current frame's own features.

**collision_suffix** (`std::string`, default `"_right"`)

Suffix appended to a feature name coming from `other` when that name already exists on
the current frame. Only the colliding feature from `other` is renamed; the current
frame's original feature keeps its unsuffixed name. Pass a different suffix (for example
`"_2"` or `"_new"`) when `"_right"` would itself collide with an existing feature name,
or when it does not read well for your domain (`"_prior_year"` for a year-over-year
join, for instance).

## Returns

`DataFrame`
    A new frame with the current frame's features first, followed by `other`'s features
    in order, with any name collisions in `other` renamed using `collision_suffix`. The
    case axis of the result matches the current frame's case axis.

## Examples

Example 1. Attach warehouse and shipping metadata onto an orders frame, with no
overlapping feature names.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id", Thread(pluss::make_series({101, 102, 103}))},
    {"city",     Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
    {"units",    Thread(pluss::make_series({2, 1, 4}))},
});

auto logistics = DataFrame({
    {"warehouse",  Thread(pluss::make_note({"WH-Accra", "WH-Tema", "WH-Kumasi"}))},
    {"ship_days",  Thread(pluss::make_series({2, 3, 5}))},
});

auto out = orders.join(logistics);
std::cout << out;
```

### Output

```text
====================================================
     | order_id  city    units  warehouse  ship_days
----------------------------------------------------
0    | 101       Accra   2      WH-Accra   2        
1    | 102       Tema    1      WH-Tema    3        
2    | 103       Kumasi  4      WH-Kumasi  5        
----------------------------------------------------
[3 cs x 5 feat]
```

`logistics` has no explicit relationship to `orders` beyond having the same number of
cases in the same order — case `0` of `logistics` ("WH-Accra") lands on case `0` of
`orders` ("Accra") purely because they occupy the same position. If `logistics` had been
built or filtered independently and its rows were not already in the same order as
`orders`, the join would silently pair the wrong rows together; `join()` has no way to
detect that for you.

Example 2. Feature name collisions get the suffix.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id", Thread(pluss::make_series({101, 102, 103}))},
    {"city",     Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
    {"units",    Thread(pluss::make_series({2, 1, 4}))},
});

auto region_lookup = DataFrame({
    {"city",       Thread(pluss::make_note({"Accra Region", "Tema Region", "Kumasi Region"}))},
    {"ship_days",  Thread(pluss::make_series({2, 3, 5}))},
});

auto out = orders.join(region_lookup);  // collision_suffix defaults to "_right"
std::cout << out;
```

### Output

```text
========================================================
     | order_id  city    units  city_right     ship_days
--------------------------------------------------------
0    | 101       Accra   2      Accra Region   2        
1    | 102       Tema    1      Tema Region    3        
2    | 103       Kumasi  4      Kumasi Region  5        
--------------------------------------------------------
[3 cs x 5 feat]
```

`orders`'s own `city` feature is untouched; only `region_lookup`'s colliding `city`
column was renamed to `city_right`. Pass an explicit `collision_suffix` (for example
`orders.join(region_lookup, "_lookup")`) if `"_right"` is not descriptive enough for the
frame you are attaching.

## Notes

- `join()` is non-mutating: the current frame and `other` are both left untouched, and a
  new frame is returned.
- Because alignment is positional, `join()` is cheap — it does not need to hash or sort
  either frame's case axis the way a key-based merge would.
- If you need to combine frames that are *not* already in matching case order — the
  common "match customers across two tables by ID" scenario — build the relationship
  explicitly first (for example by sorting or re-indexing both frames onto the same key
  order, or using [`sql()`](sql.md) for an actual key-based join) rather than calling
  `join()` directly on mismatched frames.
- `join()` does not deduplicate or validate that `other` has the same number of cases as
  the current frame; mismatched case counts are a programming error to avoid upstream,
  not something `join()` corrects for you.

## See Also

- [`DataFrame::sql`](sql.md) / [`DataFrame::prep_sql`](prep_sql.md): key-based joins
  expressed as SQL, when rows need to be matched by value rather than by position.
- [`DataFrame::assign`](assign.md): add one or a few computed features instead of
  attaching an entire second frame.
- [`DataFrame::conform`](conform.md): align a frame's case axis to another frame's case
  axis, which is often the right step to take *before* a positional `join()`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnull.html">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
  </a>
  <a class="next-link" href="kindelize.html">
    <span class="label">Next</span>
    <span class="title">kindelize</span>
  </a>
</div>
