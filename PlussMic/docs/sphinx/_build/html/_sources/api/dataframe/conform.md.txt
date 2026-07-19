<!-- pluss-hand-authored -->

# DataFrame::conform

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::conform(const DataFrame& other) const -> DataFrame
```

## What It Does

Reshapes this frame to structurally match `other`, in two steps: first, its features
are reordered (never renamed or dropped) to exactly match `other.features()`'s name
order — this requires both frames to have the *same number* of features, and every
name in `other`'s feature list must exist verbatim on this frame, or the call throws.
Second, the reordered frame's cases are reindexed to `other`'s exact case-label order,
via the same by-label matching [`align()`](align.md) uses (with no `fill`, so any
label present in `other` but absent from this frame becomes a `NULL` row).

`conform()` is the one-call version of "make my frame line up with this other frame's
shape" — useful right before an operation like [`land()`](land.md)/[`rand()`](rand.md)
that checks structural correspondence (feature order and case count) rather than
matching by label itself, or simply to present two related frames with an identical
row and column order for a side-by-side report.

## Parameters

**other** (`const DataFrame&`)

The reference frame whose structure (feature order and case-label order) this frame
should be reshaped to match. `other`'s own *values* are irrelevant to the result —
only its `features()` list and `label().values()` axis are consulted.

## Returns

`DataFrame`
    A new frame with `other`'s feature order and `other`'s case order. Feature count
    must match `other`'s exactly; case labels absent from this frame appear as `NULL`
    rows, matching `align()`'s no-fill behavior.

## Examples

Example 1. Reorder features and rows to match a reference frame with a different
column order and a different (reordered) subset of case labels.

```cpp
using namespace pluss::table;

auto raw = DataFrame({
    {"b", Thread(pluss::make_series({1, 2, 3}))},
    {"a", Thread(pluss::make_series({10, 20, 30}))},
    {"c", Thread(pluss::make_note({"p", "q", "r"}))},
}, Thread(pluss::make_note({"x", "y", "z"})));
std::cout << raw;
```

### Output

```text
=======================
     | b     a     c   
-----------------------
x    | 1     10    p   
y    | 2     20    q   
z    | 3     30    r   
-----------------------
[3 cs x 3 feat]
```

```cpp
auto reference = DataFrame({
    {"a", Thread(pluss::make_series({0, 0, 0}))},
    {"b", Thread(pluss::make_series({0, 0, 0}))},
    {"c", Thread(pluss::make_note({"", "", ""}))},
}, Thread(pluss::make_note({"z", "x", "y"})));

auto conformed = raw.conform(reference);
std::cout << conformed;
```

### Output

```text
=======================
     | a     b     c   
-----------------------
z    | 30    3     r   
x    | 10    1     p   
y    | 20    2     q   
-----------------------
[3 cs x 3 feat]
```

`reference`'s own values (all zeros and empty strings) never appear in the result —
only its feature *order* (`a, b, c`) and case-label *order* (`z, x, y`) matter. The
output carries `raw`'s original values, reordered on both axes: row `z` is `raw`'s
third row (`30, 3, r`), now listed first because `reference` lists `z` first.

Example 2. A case label present in `other` but absent from this frame becomes a
`NULL` row, following the same no-fill rule as `align(new_label)`.

```cpp
auto raw2 = DataFrame({
    {"a", Thread(pluss::make_series({1, 2}))},
    {"b", Thread(pluss::make_series({10, 20}))},
}, Thread(pluss::make_note({"x", "y"})));

auto reference2 = DataFrame({
    {"a", Thread(pluss::make_series({0, 0, 0}))},
    {"b", Thread(pluss::make_series({0, 0, 0}))},
}, Thread(pluss::make_note({"x", "z", "y"})));

auto conformed2 = raw2.conform(reference2);
std::cout << conformed2;
```

### Output

```text
=================
     | a     b   
-----------------
x    | 1     10  
z    | NULL  NULL
y    | 2     20  
-----------------
[3 cs x 2 feat]
```

`reference2` lists case `z` between `x` and `y`, but `raw2` only ever had `x` and
`y`. `conform()`'s case-reindexing step matches `x` and `y` by label as usual and
inserts `z` as a brand-new row, `NULL` in every feature — the feature-count check
only requires the two frames to have the *same number* of features, it says
nothing about the case axis lining up too.

Example 3. Name a feature in `other` that this frame doesn't have, to see the
call throw.

```cpp
try {
  auto bad_reference = DataFrame({
      {"a", Thread(pluss::make_series({0, 0}))},
      {"c", Thread(pluss::make_series({0, 0}))},
  }, Thread(pluss::make_note({"x", "y"})));
  auto out = raw2.conform(bad_reference);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrame::conform: missing feature 'c'
```

`raw2` only has `a` and `b`; `bad_reference` lists `a` and `c`, and `c` doesn't
exist on `raw2` at all, so the call rejects it before reordering anything.

## Notes

- Both frames must have exactly the same number of features, and every name in
  `other`'s feature list must already exist on this frame — `conform()` reorders, it
  never introduces or drops features the way [`align()`](align.md)'s `AlignSpec` can.
- A duplicate name in `other`'s feature list throws
  `"DataFrame::conform: duplicate feature in target '<name>'"`, and a name that
  doesn't exist on this frame throws `"DataFrame::conform: missing feature '<name>'"`.
  In practice, only the second is reachable through normal construction: a real
  `DataFrame` can never actually hold two columns with the same name in the first
  place (attempting it throws `"DataFrame: duplicate column name '<name>'"` at
  construction time), so `other.features()` can never itself contain a duplicate by
  the time `conform()` sees it.
- Case reindexing follows exactly the same by-label, no-fill rules as
  [`DataFrame::align(new_label)`](align.md) — a case label present in `other` but
  absent from this frame becomes a `NULL` row rather than an error.
- `conform()` is purely structural — it never combines or merges values between the
  two frames the way [`combine()`](combine.md) does.

## See Also

- [`DataFrame::align`](align.md): the lower-level, single-axis alignment primitive
  `conform()`'s case-reindexing step is built on.
- [`DataFrame::combine`](combine.md): actually merge two frames' values together,
  once they're already structurally compatible.
- [`DataFrame::reorder`](reorder.md): reorder features by an explicit name list
  without also touching the case axis.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="compare.html">
    <span class="label">Previous</span>
    <span class="title">compare</span>
  </a>
  <a class="next-link" href="copy.html">
    <span class="label">Next</span>
    <span class="title">copy</span>
  </a>
</div>
