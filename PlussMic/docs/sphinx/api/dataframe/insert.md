<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fb5bcf01977a -->

# DataFrame::insert

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::insert(std::int64_t feature_index, std::string name, const Thread& feature_value) const -> DataFrame
```

## What It Does

Adds one new feature at an explicit column position, shifting every feature already at
or past that position one slot to the right, and returns the resulting frame.
[`place()`](place.md) builds on this directly — it computes a position relative to an
existing "anchor" feature and calls `insert()` internally when the target name doesn't
already exist.

## Parameters

**feature_index** (`std::int64_t`)

The zero-based position the new feature should occupy. Clamped into the valid range
automatically: a value `<= 0` prepends the feature as the new first column, and a value
`>= nfeatures()` appends it as the new last column — there is no bounds error for an
out-of-range index, only clamping.

**name** (`std::string`)

The new feature's name. Must not be empty, and must not already exist on the frame —
both throw (`"DataFrame::insert: feature name cannot be empty"` and
`"DataFrame::insert: duplicate feature name '<name>'"` respectively). Use
[`set_features()`](set_features.md) instead when you want to *replace* an existing
feature's values rather than add a new one.

**feature_value** (`const Thread&`)

The new feature's values. Must be defined, and — unless the frame currently has zero
features — must have exactly `ncases()` entries; throws
`"DataFrame::insert: case count mismatch"` otherwise.

## Returns

`DataFrame`
    A new frame with `name` inserted at the (clamped) requested position, every other
    feature's relative order preserved, and the same case axis as the source.

## Examples

Example 1. Insert a new feature in the middle of the frame.

```cpp
using namespace pluss::table;

auto students = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"score", Thread(pluss::make_series({88.5, 92.0, 79.5}))},
});
std::cout << students;
```

### Output (before)

```text
==================
     | id    score
------------------
0    | 1     88.5 
1    | 2     92   
2    | 3     79.5 
------------------
[3 cs x 2 feat]
```

```cpp
auto with_grade = students.insert(1, "grade", Thread(pluss::make_note({"B", "A", "C"})));
std::cout << with_grade;
```

### Output (after)

```text
=========================
     | id    grade  score
-------------------------
0    | 1     B      88.5 
1    | 2     A      92   
2    | 3     C      79.5 
-------------------------
[3 cs x 3 feat]
```

`grade` lands at position `1`, between `id` (position `0`) and `score` (shifted from
position `1` to `2`).

Example 2. An out-of-range index is clamped, not rejected — `0` prepends, and an index
past the end appends.

```cpp
auto prepended = students.insert(0, "region", Thread(pluss::make_note({"East", "West", "East"})));
std::cout << prepended;
```

### Output

```text
==========================
     | region  id    score
--------------------------
0    | East    1     88.5 
1    | West    2     92   
2    | East    3     79.5 
--------------------------
[3 cs x 3 feat]
```

```cpp
auto appended = students.insert(100, "flag", Thread(pluss::make_series({true, false, true})));
std::cout << appended;
```

### Output

```text
=========================
     | id    score  flag 
-------------------------
0    | 1     88.5   true 
1    | 2     92     false
2    | 3     79.5   true 
-------------------------
[3 cs x 3 feat]
```

`feature_index = 100` is well past `nfeatures() = 2`, so it's clamped down to `2` —
the frame's current feature count — which places `flag` at the end, exactly as if
`insert(nfeatures(), ...)` had been called directly.

## Notes

- An out-of-range `feature_index` is silently clamped into `[0, nfeatures()]`; it is
  never a validation error on its own.
- `name` colliding with an existing feature *is* a validation error — `insert()` never
  overwrites an existing column; use [`set_features()`](set_features.md) for that.
- `feature_value`'s length must match `ncases()` unless the frame currently has no
  features at all (in which case the new feature also defines the frame's case count
  going forward).
- `insert()` is non-mutating; it always returns a new frame.

## See Also

- [`DataFrame::place`](place.md): position a feature relative to an existing "anchor"
  feature by name, rather than by a raw numeric index.
- [`DataFrame::set_features`](set_features.md): replace an *existing* feature's values
  in place of adding a new one.
- [`DataFrame::drop`](drop.md): the inverse operation — remove a named feature instead
  of adding one.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="info_string.html">
    <span class="label">Previous</span>
    <span class="title">info_string</span>
  </a>
  <a class="next-link" href="interp.html">
    <span class="label">Next</span>
    <span class="title">interp</span>
  </a>
</div>
