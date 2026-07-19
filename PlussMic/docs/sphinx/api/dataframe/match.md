<!-- pluss-hand-authored -->

# DataFrame::match

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::match(const DataFrame& other, const FrameMatchOptions& options = {}) const -> FrameMatch
```

## What It Does

Aligns two frames onto one shared case axis **and** one shared feature set at the same
time, returning both aligned frames as a `FrameMatch{left, right}` pair — ready for a
row-by-row, column-by-column comparison or computation between them. This is the
alignment step that every `DataFrame`-vs-`DataFrame` arithmetic and comparison method
(`add(other)`, `eq(other)`, and so on) already performs internally; `match()` exposes
it directly for when you want the two aligned frames themselves rather than an
immediate arithmetic result.

Cases are matched by **label** (not by position): a case label present on only one
side is padded with `NULL` on the other, per `options.cases`. Features are matched by
**name** the same way, per `options.features`. Both default to `FrameMatchRule::Unite`
— keep this frame's cases/features in order, then append anything unique to `other`.

## Parameters

**other** (`const DataFrame&`)

The frame to align against.

**options** (`const FrameMatchOptions&`, optional)

- **`cases`** (`FrameMatchRule`, default `Unite`): how the two case-label axes are
  reconciled — `Unite` (union, this frame's order first), `Common` (intersection
  only), `KeepLeft` (this frame's cases exactly), `KeepRight` (`other`'s cases
  exactly).
- **`features`** (`FrameMatchRule`, default `Unite`): the same four rules, applied to
  feature names instead of case labels.
- **`repeated_cases`** (`RepeatedCaseRule`, default `PairInOrder`): how a case label
  that appears more than once (on either side) is handled — `PairInOrder` pairs
  repeated occurrences positionally within each label group; `Reject` throws instead.

## Returns

`FrameMatch`
    A `{left, right}` pair of frames sharing the identical case axis and feature set,
    ready for direct cell-by-cell comparison.

## Examples

Example 1. Align two frames whose cases and features only partially overlap, using
the default `Unite` rule on both axes.

```cpp
using namespace pluss::table;

auto left = DataFrame({
    {"score", Thread(pluss::make_series({88.0, 92.0, 79.0}))},
}, Thread(pluss::make_note({"a", "b", "c"})));

auto right = DataFrame({
    {"score", Thread(pluss::make_series({10.0, 20.0}))},
    {"bonus", Thread(pluss::make_series({5.0, 8.0}))},
}, Thread(pluss::make_note({"b", "d"})));

std::cout << left;
```

### Output (left, before)

```text
============
     | score
------------
a    | 88   
b    | 92   
c    | 79   
------------
[3 cs x 1 feat]
```

```cpp
std::cout << right;
```

### Output (right, before)

```text
===================
     | score  bonus
-------------------
b    | 10     5    
d    | 20     8    
-------------------
[2 cs x 2 feat]
```

```cpp
auto matched = left.match(right);
std::cout << matched.left;
std::cout << matched.right;
```

### Output (matched.left)

```text
===================
     | score  bonus
-------------------
a    | 88     NULL 
b    | 92     NULL 
c    | 79     NULL 
d    | NULL   NULL 
-------------------
[4 cs x 2 feat]
```

### Output (matched.right)

```text
===================
     | score  bonus
-------------------
a    | NULL   NULL 
b    | 10     5    
c    | NULL   NULL 
d    | 20     8    
-------------------
[4 cs x 2 feat]
```

Both results share the identical case axis (`a, b, c, d` — `left`'s cases first, then
`right`'s only-case `d`) and identical feature set (`score, bonus` — `left`'s features
first, then `right`'s only-feature `bonus`). `matched.left` has `NULL` for `bonus`
everywhere (it never had that feature) and `NULL` for `score` at `d` (it never had
that case); `matched.right` mirrors this the other way around. Only case `b`, present
on both sides, has real values from both frames after matching.

## Notes

- Matching is by **label**, not position — two frames with cases in a different order
  (or a different count) still align correctly as long as the shared labels match.
- `match()` is the exact operation every `DataFrame`-vs-`DataFrame` arithmetic and
  comparison overload runs before combining values; reach for it directly when you
  need the aligned pair itself, not just the combined result.
- A repeated case label (the same label more than once, on either side) is paired by
  occurrence order by default (`RepeatedCaseRule::PairInOrder`); pass
  `RepeatedCaseRule::Reject` to throw instead of silently pairing by position.

## See Also

- [`DataFrame::add`](add.md), [`DataFrame::eq`](eq.md): the arithmetic and comparison
  families that call `match()` internally before combining two frames.
- [`DataFrame::extend`](extend.md): stack two frames' *cases* together instead of
  aligning them side by side.
- [`DataFrame::conform`](conform.md): reshape one frame's structure to match another's
  more directly, when a full `FrameMatch` pair isn't what you need.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mask.html">
    <span class="label">Previous</span>
    <span class="title">mask</span>
  </a>
  <a class="next-link" href="matmul.html">
    <span class="label">Next</span>
    <span class="title">matmul</span>
  </a>
</div>
