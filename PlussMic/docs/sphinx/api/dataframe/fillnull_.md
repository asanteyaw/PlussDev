<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c3243745ae01 -->

# DataFrame::fillnull_

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame& DataFrame::fillnull_(double value)
DataFrame& DataFrame::fillnull_(std::string_view value)
DataFrame& DataFrame::fillnull_(const char* value)
DataFrame& DataFrame::fillnull_(const pluss::Timeline& value)
DataFrame& DataFrame::fillnull_(const pluss::Duration& value)
```

## What It Does

The in-place, whole-frame counterpart to [`fillnull()`](fillnull.md). Where
`fillnull()` is `const` and lets you target specific features and specific fill
strategies (forward-fill, backward-fill, a per-feature value) through a `MissingSpec`,
`fillnull_()` is deliberately narrower and blunter: pick the overload matching a
column *kind*, and it fills **every** feature of that kind on the frame with the same
scalar `value`, mutating the frame directly and returning `*this` by reference.

```{important} There is no features parameter
Unlike `fillnull()`, none of the `fillnull_()` overloads accept a feature list. Calling
`df.fillnull_(0.0)` fills every `Series` feature's nulls with `0.0` — you cannot target
just one numeric column with this method. If you need a single-feature fill, either use
[`fillnull()`](fillnull.md) with an explicit `features` list, or reassign the one column
directly (`df.features("price").fillnull(0.0)` and splice it back with
[`insert()`](insert.md) / [`place()`](place.md)).
```

Each overload only touches features of the matching kind:

- `fillnull_(double)`: every `Series` feature.
- `fillnull_(std::string_view)` / `fillnull_(const char*)`: every `Note` feature.
- `fillnull_(const pluss::Timeline&)`: every `Timeline` feature.
- `fillnull_(const pluss::Duration&)`: every `Duration` feature.

A feature of a *different* kind than the overload you called is left completely
untouched — calling `fillnull_(0.0)` never affects a `Note` column, even if that
column also has missing values.

## Parameters

**value**

The replacement value, in whichever type matches the overload: a `double` for numeric
`Series` features, a string for `Note` features, a `Timeline` scalar for `Timeline`
features, or a `Duration` scalar for `Duration` features. The same `value` is used for
every matching feature on the frame — there is no per-feature value list.

## Returns

`DataFrame&`
    A reference to the same frame, now mutated in place. Every feature of the matching
    kind has had its nulls replaced; every feature of a different kind is unchanged.

## Examples

Example 1. Fill every numeric feature's nulls with `0.0` in one call — note that both
numeric columns are affected, not just one.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"temp",     Slate(pluss::make_series({68.5, std::numeric_limits<double>::quiet_NaN(), 71.2}),
                        pluss::make_series({true, false, true}, pluss::ScalarType::Bool))},
    {"humidity", Slate(pluss::make_series({30.0, 40.0, std::numeric_limits<double>::quiet_NaN()}),
                        pluss::make_series({true, true, false}, pluss::ScalarType::Bool))},
});
std::cout << readings;
```

### Output (before)

```text
=====================
     | temp  humidity
---------------------
0    | 68.5  30      
1    | NULL  40      
2    | 71.2  NULL    
---------------------
[3 cs x 2 feat]
```

```cpp
readings.fillnull_(0.0);
std::cout << readings;
```

### Output (after)

```text
=====================
     | temp  humidity
---------------------
0    | 68.5  30      
1    | 0     40      
2    | 71.2  0       
---------------------
[3 cs x 2 feat]
```

Both `temp`'s null (case `1`) and `humidity`'s null (case `2`) are filled with the same
`0.0` in a single call — `fillnull_(double)` never singles out one numeric column.

Example 2. Fill a `Note` feature's nulls; a numeric feature with its own null is left
completely alone, since it isn't the kind this overload targets.

```cpp
auto orders = DataFrame({
    {"city",  Slate(pluss::make_note({"Accra", "", "Tema"}),
                     pluss::make_series({true, false, true}, pluss::ScalarType::Bool))},
    {"score", Slate(pluss::make_series({88.5, std::numeric_limits<double>::quiet_NaN(), 79.5}),
                     pluss::make_series({true, false, true}, pluss::ScalarType::Bool))},
});
std::cout << orders;
```

### Output (before)

```text
===================
     | city   score
-------------------
0    | Accra  88.5 
1    | NULL   NULL 
2    | Tema   79.5 
-------------------
[3 cs x 2 feat]
```

```cpp
orders.fillnull_("Unknown");
std::cout << orders;
```

### Output (after)

```text
=====================
     | city     score
---------------------
0    | Accra    88.5 
1    | Unknown  NULL 
2    | Tema     79.5 
---------------------
[3 cs x 2 feat]
```

Only `city` (a `Note` feature) changed; `score`'s own null at case `1` is completely
untouched, since `fillnull_(std::string_view)` never looks at `Series` columns at all
— a second call to `fillnull_(0.0)` would be needed to fill it.

## Notes

- These overloads mutate the frame in place and return `DataFrame&` — the only mutating
  members in the missing-data family. [`fillnull()`](fillnull.md) is the `const`,
  non-mutating alternative that also supports per-feature targeting and forward/backward
  fill strategies.
- There is no feature-selection parameter on any `fillnull_()` overload; the fill always
  applies to every feature of the matching kind.
- A feature whose kind doesn't match the overload called is left completely alone, even
  if it has nulls of its own — call the matching overload for each kind you need filled.
- The `pluss::Timeline` and `pluss::Duration` overloads follow the exact same
  kind-scoped, whole-frame, mutating pattern as the `double` and `std::string_view`
  overloads shown above, filling every `Timeline` or `Duration` feature's nulls with
  the given scalar.

## See Also

- [`DataFrame::fillnull`](fillnull.md): the `const`, per-feature, multi-strategy
  alternative (constant value, forward-fill, or backward-fill via `MissingSpec`).
- [`DataFrame::ffill`](ffill.md) / [`DataFrame::bfill`](bfill.md): propagate a
  neighboring observation instead of writing a fixed scalar.
- [`DataFrame::isnull`](isnull.md): locate missing cells before deciding how to fill
  them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillnull.html">
    <span class="label">Previous</span>
    <span class="title">fillnull</span>
  </a>
  <a class="next-link" href="filter.html">
    <span class="label">Next</span>
    <span class="title">filter</span>
  </a>
</div>
