<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0cc450dbded1 -->

# DataFrameGrouped::mode_ties

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::mode_ties(std::string_view feature, std::string_view output = "") const -> DataFrame
```

## What It Does

Returns **every** value tied for the highest count within each group, instead of the
single winner [`mode()`](mode.md) picks. This is a standalone terminal, not a
`GroupSpec`/[`agg()`](agg.md) operation — a tie can legitimately produce more than
one output row for a group, and `agg()`/`transform()` depend on every spec in a
single call agreeing on exactly one row per group, so this can't be expressed as a
spec alongside them.

Ties are broken by sorting the tied values themselves in ascending order (not by
which one was observed first) — this deliberately does *not* reuse the
first-occurrence tie-break [`mode()`](mode.md) and [`Slate::freq()`](../slate/freq.md)
use, because those pick one arbitrary winner among many distinct values, while this
method already returns the full tied set: ordering that set by value is a property of
the values themselves, independent of how the source rows happened to be ordered,
which makes the result reproducible even if the input row order changes.

A group with zero observed (non-null) values for `feature` still produces exactly
one row, with a null value — the same one-row-per-group-at-minimum guarantee every
other grouped terminal makes, even though this method's row count is not fixed at
exactly one.

## Parameters

**feature** (`std::string_view`)

The single value feature to compute tied modes for. Unlike the zero-argument
terminals ([`count()`](count.md), [`sum()`](sum.md), [`mode()`](mode.md), and so
on), this method never applies itself to every remaining value feature at once —
combining several features' tie sets would require an arbitrary cross-product rule,
so it is intentionally scoped to one feature per call. Call it once per feature and
join the results if you need several.

**output** (`std::string_view`, optional)

Output column name. Defaults to `"{feature}_mode_ties"` when empty — deliberately
distinct from `mode()`'s `"{feature}_mode"` default, so the two are hard to confuse
even skimming a schema.

## Returns

`DataFrame`
    One row per **tied value**, not one row per group. The group key column(s)
    come first, followed by one output column holding the tied value for that row.
    A group contributes as many rows as it has tied values, or exactly one null row
    if it had no observed values at all.

## Examples

Example 1. Contrast with `mode()`: no tie stays one row, a tie becomes two.

```cpp
using namespace pluss::table;

auto loans = DataFrame({
    {"region", Slate(pluss::make_note({"North", "North", "South", "South"}))},
    {"status", Slate(pluss::make_note({"good", "good", "bad", "good"}))},
});
std::cout << loans;

auto out = loans.group().by({"region"}).mode_ties("status");
std::cout << out;
```

### Output (loans)

```text
=====================
     | region  status
---------------------
0    | North   good  
1    | North   good  
2    | South   bad   
3    | South   good  
---------------------
[4 cs x 2 feat]
```

### Output (out)

```text
===============================
     | region  status_mode_ties
-------------------------------
0    | North   good            
1    | South   bad             
2    | South   good            
-------------------------------
[3 cs x 2 feat]
```

`"North"`'s two statuses are both `"good"`, so it contributes exactly one row.
`"South"` is tied between `"bad"` and `"good"` (one occurrence each), so it
contributes two rows — both tied values, sorted ascending (`"bad"` before
`"good"`) — rather than [`mode()`](mode.md)'s single first-occurrence pick of
`"bad"` for the same data.

Example 2. A group with no observed values still produces one row, with a null.

```cpp
auto readings = DataFrame({
    {"region", Slate(pluss::make_note({"North", "North", "South", "South"}))},
    {"status", Slate(pluss::make_note(
        {"x", "x", "bad", "good"}),
        pluss::make_series(std::vector<bool>{false, false, true, true}))},
});
std::cout << readings;

auto out = readings.group().by({"region"}).mode_ties("status");
std::cout << out;
```

### Output (readings)

```text
=====================
     | region  status
---------------------
0    | North   NULL  
1    | North   NULL  
2    | South   bad   
3    | South   good  
---------------------
[4 cs x 2 feat]
```

### Output (out)

```text
===============================
     | region  status_mode_ties
-------------------------------
0    | North   NULL            
1    | South   bad             
2    | South   good            
-------------------------------
[3 cs x 2 feat]
```

`"North"` has no valid `status` values at all, so it still contributes exactly one
row — with a null value, not zero rows. Dropping the group entirely instead would
break the invariant every other grouped terminal relies on: that every group key
appears at least once in the output, which matters if downstream code joins this
result back onto a full list of groups.

Example 3. Works on numeric value features too, not just `Note`.

```cpp
auto readings3 = DataFrame({
    {"region", Slate(pluss::make_note({"A", "A", "A", "B", "B", "B", "B"}))},
    {"amount", Slate(pluss::make_series({10.0, 20.0, 20.0, 5.0, 5.0, 9.0, 9.0}))},
});
std::cout << readings3;

auto out3 = readings3.group().by({"region"}).mode_ties("amount");
std::cout << out3;
```

### Output (readings3)

```text
=====================
     | region  amount
---------------------
0    | A       10    
1    | A       20    
2    | A       20    
3    | B       5     
4    | B       5     
5    | B       9     
6    | B       9     
---------------------
[7 cs x 2 feat]
```

### Output (out3)

```text
===============================
     | region  amount_mode_ties
-------------------------------
0    | A       20              
1    | B       5               
2    | B       9               
-------------------------------
[3 cs x 2 feat]
```

`"A"`'s `20` appears twice against `10`'s one occurrence, so `"A"` has a single,
unambiguous mode. `"B"` is tied between `5` and `9` (two occurrences each), so it
contributes both, sorted ascending numerically — not lexicographically, which
matters once values reach double digits (`"9" < "20"` as strings, but `9 < 20` as
numbers; this method always sorts by the latter for numeric features).

## Notes

- This is a standalone method, not composable with [`agg()`](agg.md) or
  [`transform()`](transform.md) in the same call — those require a fixed one-row-
  per-group shape across every spec, which a variable-length tie set cannot satisfy.
- Ties are sorted ascending by value, not by first-occurrence order — a deliberate
  difference from [`mode()`](mode.md) and [`Slate::freq()`](../slate/freq.md),
  justified by this method returning the whole tied set rather than picking one
  winner from it.
- Supports the same value-feature kinds as [`mode()`](mode.md): `Note` and numeric
  `Series`.
- `dropnull` is not a parameter here — it inherits from the `GroupOptions` already
  captured by [`group()`](../dataframe/group.md), the same as every other grouped
  terminal.
- Scoped to exactly one feature per call, unlike the zero-argument convenience
  terminals — this is intentional, not a limitation to be lifted later, since
  combining several features' tie sets has no single obviously-correct behavior.

## See Also

- [`DataFrameGrouped::mode`](mode.md): the single-winner counterpart this method
  complements.
- [`Slate::freq()`](../slate/freq.md): the same first-occurrence tie-break
  `mode()` uses, explained in full.
- [`DataFrameGrouped::agg`](agg.md): the fixed-one-row-per-group terminal this
  method cannot be expressed through.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mode.html">
    <span class="label">Previous</span>
    <span class="title">mode</span>
  </a>
</div>
