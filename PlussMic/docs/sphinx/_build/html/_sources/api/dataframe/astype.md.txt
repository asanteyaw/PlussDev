<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0e9e783a333d -->

# DataFrame::astype

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::astype(pluss::RangeView<std::pair<std::string_view, pluss::ScalarType>> feature_dtypes) -> DataFrame
DataFrame::astype(pluss::RangeView<std::string_view> features, pluss::ScalarType dtype) -> DataFrame
DataFrame::astype(std::initializer_list<std::pair<std::string_view, pluss::ScalarType>> feature_dtypes) -> DataFrame
```

## What It Does

Casts one or more existing features to new scalar dtypes — the typed conversion surface
for turning imported or computed columns into the representation later code actually
needs — normalizing a
column that arrived as `float64` but is really integer counts, converting a 0/1 numeric
flag into a proper boolean before it's used in filtering, or tightening precision once a
computation is done and the extra headroom is no longer needed.

Two shapes are offered depending on whether every selected feature shares one target
dtype or each needs its own:

- **Shared-dtype overload** (`features`, `dtype`): cast a list of feature names to the
  same dtype in one call — the common case when several columns arrived with the same
  wrong type.
- **Mapping overload** (`feature_dtypes`, a list or initializer-list of
  `(feature_name, dtype)` pairs): cast each selected feature to its own dtype, when the
  targets differ feature by feature.

Either way, `astype()` returns a new frame; features not mentioned in the request pass
through completely unchanged.

## Parameters

**features** (`pluss::RangeView<std::string_view>`)

Feature names to cast when every one of them should land on the same target `dtype`.
Features not listed are left unchanged.

**dtype** (`pluss::ScalarType`)

Shared target scalar dtype for the feature-list overload. Common choices include
`pluss::ScalarType::Bool`, `pluss::ScalarType::Int64`, and `pluss::ScalarType::Float64`.
Pick the narrowest dtype that still matches what downstream code plans to do with the
column — casting to `Bool` too early can throw away information a later filter still
needed as a raw number, for instance.

**feature_dtypes** (`pluss::RangeView<std::pair<std::string_view, pluss::ScalarType>>` /
`std::initializer_list<std::pair<std::string_view, pluss::ScalarType>>`)

A collection of `(feature_name, dtype)` pairs for mixed casting, one target dtype per
listed feature. Use this overload once the shared-dtype overload is no longer expressive
enough — for example casting an ID column to `Int64` while casting a flag column to
`Bool` in the same call.

## Returns

`DataFrame`
    A new frame with the requested feature casts applied. Features not mentioned in the
    cast request are preserved unchanged, including their original dtype.

## Examples

Example 1. Cast one selected feature to a shared target dtype.

```cpp
using namespace pluss::table;

auto raw = DataFrame({
    {"student_id", pluss::make_series({101.0, 102.0, 103.0})},
    {"passed",     pluss::make_series({1.0, 0.0, 1.0})},
}).title("raw");

auto out = raw.astype({"passed"}, pluss::ScalarType::Bool);
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

`passed` arrived as a raw `float64` 0.0/1.0 flag — a common shape for numeric data
loaded from CSV or a database export where booleans have no native representation.
Casting it to `Bool` here makes its intent explicit and lets later code branch on it
directly (`if (row["passed"])`) instead of comparing against `1.0` every time.
`student_id` was not listed, so it keeps its original `float64` dtype untouched.

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

Example 3. Name a feature that doesn't exist on the frame, to see it throw.

```cpp
try {
  auto out = raw.astype({"not_a_column"}, pluss::ScalarType::Bool);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrame::cast: unknown feature 'not_a_column'
```

`raw` only has `student_id` and `passed` — `astype()` delegates to
[`cast()`](cast.md) internally, which rejects an unknown target feature name
rather than silently skipping it.

## Notes

- Use the feature-list overload (`astype({"a", "b"}, dtype)`) when several features
  should land on the same target dtype; use the mapping overload
  (`astype({{"a", dtype_a}, {"b", dtype_b}})`) when each feature needs its own.
- This method returns a new frame; it does not mutate the current `DataFrame` in place.
- Casting does not validate that the conversion is lossless — casting a `Float64` column
  with fractional values to `Int64` truncates, the same way a `static_cast<int>` would.
  Confirm the source values are already integral before casting for cleanliness rather
  than for rounding.
- For casting an entire feature onto a different *payload family* (for example forcing a
  numeric-looking column onto the `Note` text family before string operations), see
  [`cast()`](cast.md) instead — `astype()` changes the scalar dtype within a feature's
  existing payload family, not the family itself. `cast()`'s `CastSpec` overload accepts a
  `ThreadCastTarget` (e.g. `ThreadCastTarget::Note`) for exactly that purpose.

## See Also

- [`DataFrame::cast`](cast.md): the broader casting route — change a feature's payload
  family (`Series`/`Note`/etc.), not just its scalar dtype within the current family.
- [`Thread::astype`](../thread/astype.md): cast one thread directly outside a frame.

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
