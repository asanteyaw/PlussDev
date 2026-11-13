<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9c1708ae7abb -->

# DataFrame::update

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame DataFrame::update(const DataFrame& other, bool overwrite = true) const
```

## What It Does

A thin, case-label-matched wrapper over [`combine()`](combine.md):
`update(other, overwrite)` is exactly `combine(other, {.op = CombineOp::Update,
.overwrite = overwrite, .cases = FrameMatchRule::KeepLeft})`. It refreshes this
frame's existing feature values with `other`'s values wherever their case labels
line up — but, unlike a general `combine()` call, `update()` never introduces a
feature that doesn't already exist on this frame, and the result's case labels are
always exactly this frame's own (`FrameMatchRule::KeepLeft`), regardless of what
extra case labels `other` might carry.

## Parameters

**other** (`const DataFrame&`)

The frame supplying replacement values, matched to this frame by case label.

**overwrite** (`bool`, default `true`)

Whether a non-null value in `other` replaces a non-null value already present in
this frame. When `false`, only cells that are `NULL` in this frame get filled from
`other`; existing non-null values are left untouched either way. (In both cases, a
`NULL` in `other` never erases an existing non-null value in this frame.)

## Returns

`DataFrame`
    A new frame, same shape and case labels as this frame, with matching cases'
    feature values refreshed from `other`.

## Examples

Example 1. Refresh overlapping cases' values from another frame; case labels and
features not shared by both are left as this frame's own.

```cpp
using namespace pluss::table;

auto left = DataFrame({
    {"name", Slate(pluss::make_note({"Ama", "Kofi", "Efua"}))},
    {"age",  Slate(pluss::make_series({30.0, std::nullopt, 28.0}))},
}, Slate(pluss::make_note({"c1", "c2", "c3"})));
std::cout << left;

auto right = DataFrame({
    {"age",   Slate(pluss::make_series({26.0, 40.0, 22.0}))},
    {"score", Slate(pluss::make_series({88.0, 91.0, 75.0}))},
}, Slate(pluss::make_note({"c2", "c3", "c4"})));
std::cout << right;
```

### Output

```text
=================
     | name  age 
-----------------
c1   | Ama   30  
c2   | Kofi  NULL
c3   | Efua  28  
-----------------
[3 cs x 2 feat]
=========================
     | age  score
-------------------------
c2   | 26   88   
c3   | 40   91   
c4   | 22   75   
-------------------------
[3 cs x 2 feat]
```

```cpp
auto updated = left.update(right);
std::cout << updated;
```

### Output

```text
=================
     | name  age 
-----------------
c1   | Ama   30  
c2   | Kofi  26  
c3   | Efua  40  
-----------------
[3 cs x 2 feat]
```

The result keeps exactly `left`'s own case labels (`c1`, `c2`, `c3`) — `right`'s
`c4` is dropped entirely, since `update()` always uses `FrameMatchRule::KeepLeft`.
`c1`'s `age` (`30`) has no counterpart in `right`, so it's untouched. `c2`'s `age`
was `NULL` in `left` and `26` in `right`, so it's filled in. `c3`'s `age` was `28`
in `left` but `40` in `right`; with the default `overwrite = true`, `right`'s
non-null value wins. `right`'s `score` feature is ignored completely — `update()`
never adds a feature that wasn't already on `left`, unlike a general
[`combine()`](combine.md) call.

## Notes

- `update(other, overwrite)` is exactly `combine(other, {.op = CombineOp::Update,
  .overwrite = overwrite, .cases = FrameMatchRule::KeepLeft})` — see
  [`combine()`](combine.md) for the full generality this wraps.
- Only features that already exist on this frame are ever touched; anything unique
  to `other` (like `score` above) is silently ignored.
- The result's case labels are always exactly this frame's own — cases unique to
  `other` never appear in the result.
- With `overwrite = false`, a cell already holding a non-null value in this frame is
  never replaced, even if `other` has a differing non-null value for that same case.

## See Also

- [`DataFrame::combine`](combine.md): the general merge this method wraps a fixed
  configuration of.
- [`DataFrame::unify`](unify.md): a fundamentally different, key-value relational
  join rather than a case-label-matched refresh.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unstack.html">
    <span class="label">Previous</span>
    <span class="title">unstack</span>
  </a>
  <a class="next-link" href="var.html">
    <span class="label">Next</span>
    <span class="title">var</span>
  </a>
</div>
