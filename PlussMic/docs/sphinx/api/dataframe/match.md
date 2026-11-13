<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 432f8a7c95e7 -->

# DataFrame::match

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
FrameMatch DataFrame::match(const DataFrame& other, const FrameMatchOptions& options = {}) const
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
    {"score", Slate(pluss::make_series({88.0, 92.0, 79.0}))},
}, Slate(pluss::make_note({"a", "b", "c"})));

auto right = DataFrame({
    {"score", Slate(pluss::make_series({10.0, 20.0}))},
    {"bonus", Slate(pluss::make_series({5.0, 8.0}))},
}, Slate(pluss::make_note({"b", "d"})));

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

Example 2. `FrameMatchRule::Common` on both axes — keep only what's shared, dropping
everything unique to either side.

```cpp
auto common = left.match(right, {.cases = FrameMatchRule::Common,
                                  .features = FrameMatchRule::Common});
std::cout << common.left;
std::cout << common.right;
```

### Output (common.left)

```text
============
     | score
------------
b    | 92   
------------
[1 cs x 1 feat]
```

### Output (common.right)

```text
============
     | score
------------
b    | 10   
------------
[1 cs x 1 feat]
```

Only case `b` (the one label present on both sides) and `score` (the one feature name
present on both sides) survive — `bonus` and cases `a`, `c`, `d` are dropped entirely
rather than padded with `NULL`. Contrast this with Example 1's `Unite`, which keeps
everything and pads the gaps instead.

Example 3. Mixed rules — `KeepLeft` cases with `Common` features. The two axes don't
have to use the same rule.

```cpp
auto mixed = left.match(right, {.cases = FrameMatchRule::KeepLeft,
                                 .features = FrameMatchRule::Common});
std::cout << mixed.left;
std::cout << mixed.right;
```

### Output (mixed.left)

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

### Output (mixed.right)

```text
============
     | score
------------
a    | NULL 
b    | 10   
c    | NULL 
------------
[3 cs x 1 feat]
```

`KeepLeft` forces the case axis to be exactly `left`'s own cases (`a, b, c`) —
`right`'s only-case `d` never appears, and `mixed.right` gets `NULL` at `a` and `c`
since `right` never had those labels. `Common` on the feature axis drops `bonus`
from both sides, since it isn't shared.

Example 4. `FrameMatchRule::KeepRight` on both axes — the mirror image of `KeepLeft`,
forcing both the case and feature axes to exactly match `right`'s.

```cpp
auto keep_right = left.match(right, {.cases = FrameMatchRule::KeepRight,
                                      .features = FrameMatchRule::KeepRight});
std::cout << keep_right.left;
std::cout << keep_right.right;
```

### Output (keep_right.left)

```text
===================
     | score  bonus
-------------------
b    | 92     NULL 
d    | NULL   NULL 
-------------------
[2 cs x 2 feat]
```

### Output (keep_right.right)

```text
===================
     | score  bonus
-------------------
b    | 10     5    
d    | 20     8    
-------------------
[2 cs x 2 feat]
```

The case axis is now exactly `right`'s (`b, d`, dropping `left`'s only-case `a`), and
the feature axis is exactly `right`'s (`score, bonus`) — since `left` never had a
`bonus` feature at all, `keep_right.left` shows `NULL` for `bonus` on every case, not
just the unmatched ones.

## Notes

- Matching is by **label**, not position — two frames with cases in a different order
  (or a different count) still align correctly as long as the shared labels match.
- `match()` is the exact operation every `DataFrame`-vs-`DataFrame` arithmetic and
  comparison overload runs before combining values; reach for it directly when you
  need the aligned pair itself, not just the combined result.
- A repeated case label (the same label more than once, on either side) is paired by
  occurrence order by default (`RepeatedCaseRule::PairInOrder`); pass
  `RepeatedCaseRule::Reject` to throw instead of silently pairing by position.
- `cases` and `features` are independent settings — mixing rules (e.g. `KeepLeft`
  cases with `Common` features, Example 3) is perfectly valid, and each axis's rule
  only affects that axis.
- `Unite` and `KeepLeft`/`KeepRight` always produce output at least as large as their
  reference side, padding gaps with `NULL`; `Common` is the only rule that can shrink
  the result, since it drops anything not shared by both sides instead of padding it.

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
