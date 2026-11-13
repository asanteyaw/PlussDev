<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9d3ec457e720 -->

# DataFrame::astype

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame DataFrame::astype(pluss::ScalarType dtype)
DataFrame DataFrame::astype(pluss::RangeView<std::pair<std::string_view, pluss::ScalarType>> feature_dtypes)
DataFrame DataFrame::astype(std::initializer_list<std::pair<std::string_view, pluss::ScalarType>> feature_dtypes)
```

## What It Does

Casts feature(s) to new scalar dtypes — the typed conversion surface for turning
imported or computed columns into the representation later code actually needs —
normalizing a column that arrived as `float64` but is really integer counts,
converting a 0/1 numeric flag into a proper boolean before it's used in filtering, or
tightening precision once a computation is done and the extra headroom is no longer
needed.

Two shapes are offered depending on whether every feature on the current frame shares
one target dtype or each needs its own:

- **Shared-dtype overload** (`dtype` alone): cast **every** feature currently on the
  frame to the same target dtype in one call. `astype()` carries no feature selector
  of its own — narrow to the specific column(s) first with
  [`features()`](features.md) when only some columns should change:
  `raw.features({"passed"}).astype(pluss::ScalarType::Bool)`.
- **Mapping overload** (`feature_dtypes`, a list or initializer-list of
  `(feature_name, dtype)` pairs): cast named features on the *current* frame to their
  own dtype, when the targets differ feature by feature — this overload still names
  features explicitly, since it needs a distinct dtype per name.

Either way, `astype()` returns a new frame; for the shared-dtype overload, that new
frame contains only whatever features were on the current (possibly narrowed) frame
when you called it — see the Notes below for recombining a narrowed cast back into
the full original frame.

## Parameters

**dtype** (`pluss::ScalarType`)

Shared target scalar dtype applied to every feature on the current frame. Common
choices include `pluss::ScalarType::Bool`, `pluss::ScalarType::Int64`, and
`pluss::ScalarType::Float64`. Pick the narrowest dtype that still matches what
downstream code plans to do with the column — casting to `Bool` too early can throw
away information a later filter still needed as a raw number, for instance.

**feature_dtypes** (`pluss::RangeView<std::pair<std::string_view, pluss::ScalarType>>` /
`std::initializer_list<std::pair<std::string_view, pluss::ScalarType>>`)

A collection of `(feature_name, dtype)` pairs for mixed casting, one target dtype per
named feature. Use this overload once the shared-dtype overload is no longer
expressive enough — for example casting an ID column to `Int64` while casting a flag
column to `Bool` in the same call. Features not named here are left unchanged.

## Returns

`DataFrame`
    A new frame with the requested feature casts applied. Under the mapping overload,
    features not named in `feature_dtypes` are preserved unchanged, including their
    original dtype. Under the shared-dtype overload, the result contains exactly the
    features that were present on the (possibly `features()`-narrowed) frame the call
    was made on.

## Examples

Example 1. Cast one selected feature to a shared target dtype by narrowing with
[`features()`](features.md) first, then merge the cast column back into the full
frame with [`update()`](update.md).

```cpp
using namespace pluss::table;

auto raw = DataFrame({
    {"student_id", pluss::make_series({101.0, 102.0, 103.0})},
    {"passed",     pluss::make_series({1.0, 0.0, 1.0})},
}).title("raw");

auto passed_bool = raw.features({"passed"}).astype(pluss::ScalarType::Bool);
std::cout << passed_bool;
```

### Output (the narrowed, cast-only result)

```text
============
     | passed
------------
0    | true  
1    | false 
2    | true  
------------
[3 cs x 1 feat]
```

`passed_bool` only has the one narrowed feature — `student_id` isn't part of the
result at all, since `.features({"passed"})` projected it out before `astype()` ever
ran.

```cpp
auto out = raw.update(passed_bool);
std::cout << out;
```

### Output (merged back into the full frame)

```text
=========================
     | student_id  passed
-------------------------
0    | 101         true  
1    | 102         false 
2    | 103         true  
-------------------------
[3 cs x 2 feat]
```

`passed` arrived as a raw `float64` 0.0/1.0 flag — a common shape for numeric data
loaded from CSV or a database export where booleans have no native representation.
Casting it to `Bool` here makes its intent explicit and lets later code branch on it
directly (`if (row["passed"])`) instead of comparing against `1.0` every time.
[`update()`](update.md) refreshes just `passed` on `raw` — `update()` never introduces
a feature that wasn't already on the target frame, and `student_id` was never part of
`passed_bool` to begin with, so it stays exactly as it was.

Example 2. Cast several features to different dtypes in one call.

```cpp
using namespace pluss::table;

auto raw = DataFrame({
    {"student_id", pluss::make_series({101.0, 102.0, 103.0})},
    {"passed",     pluss::make_series({1.0, 0.0, 1.0})},
}).title("raw");

auto out = raw.astype({
    {"student_id", pluss::ScalarType::Int64},
    {"passed",     pluss::ScalarType::Bool},
});
std::cout << out;
```

### Output

```text
=========================
     | student_id  passed
-------------------------
0    | 101         true  
1    | 102         false 
2    | 103         true  
-------------------------
[3 cs x 2 feat]
```

Here both features needed a different target dtype: `student_id` moves from `float64`
to `Int64` (its values were always whole numbers; there was no reason to keep floating
storage), and `passed` moves to `Bool` as before. The mapping overload applies both
casts atomically in one call, rather than requiring two chained `astype()` calls.

Example 3. Name a feature that doesn't exist on the frame, to see `features()` throw
before `astype()` is ever reached.

```cpp
try {
  auto out = raw.features({"not_a_column"}).astype(pluss::ScalarType::Bool);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrame::features: unknown column 'not_a_column'
```

`raw` only has `student_id` and `passed` — the failure comes from
[`features()`](features.md) itself, rejecting the unknown name before `astype()` gets
a chance to run at all.

## Notes

- Use the shared-dtype overload (`astype(dtype)`) — narrowed to the target feature(s)
  with [`features()`](features.md) first — when several features should land on the
  same target dtype; use the mapping overload
  (`astype({{"a", dtype_a}, {"b", dtype_b}})`) directly on the full frame when each
  feature needs its own dtype.
- This method returns a new frame; it does not mutate the current `DataFrame` in place.
- The shared-dtype overload has no feature selector of its own and always casts
  *every* feature on the current frame — the result of a `features()`-narrowed call
  contains only the narrowed feature(s). Recombine with [`update()`](update.md) (as in
  Example 1) if the untouched features need to survive alongside the cast.
- Casting does not validate that the conversion is lossless — casting a `Float64` column
  with fractional values to `Int64` truncates, the same way a `static_cast<int>` would.
  Confirm the source values are already integral before casting for cleanliness rather
  than for rounding.
- For casting an entire feature onto a different *payload family* (for example forcing a
  numeric-looking column onto the `Note` text family before string operations), see
  [`cast()`](cast.md) instead — `astype()` changes the scalar dtype within a feature's
  existing payload family, not the family itself. `cast()`'s `CastSpec` overload accepts a
  `SlateCastTarget` (e.g. `SlateCastTarget::Note`) for exactly that purpose.

## See Also

- [`DataFrame::cast`](cast.md): the broader casting route — change a feature's payload
  family (`Series`/`Note`/etc.), not just its scalar dtype within the current family.
- [`Slate::astype`](../slate/astype.md): cast one slate directly outside a frame.
- [`DataFrame::features`](features.md): narrow the frame to the feature(s) the
  shared-dtype overload should cast.
- [`DataFrame::update`](update.md): merge a narrowed, cast-only result back into the
  full original frame.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="assign.html">
    <span class="label">Previous</span>
    <span class="title">assign</span>
  </a>
  <a class="next-link" href="bfill.html">
    <span class="label">Next</span>
    <span class="title">bfill</span>
  </a>
</div>
