<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 863edfaef91e -->

# DataFrame::combine

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::combine(const DataFrame& other) const -> DataFrame
DataFrame::combine(const DataFrame& other, const CombineSpec& spec) const -> DataFrame
```

## What It Does

Merges this frame with `other` along the *case* axis (matched by case-label value, the
same way [`align()`](align.md) matches rows), then decides what happens to each of
`other`'s features according to `spec.op`:

- **`CombineOp::Features`** (the default, via `combine(other)`): keep every feature
  from this frame as-is, then bring in `other`'s features too — a feature name that
  doesn't already exist is added outright; a feature name that collides with an
  existing one is added under a disambiguated name (`name + spec.collision_suffix`,
  default `"_right"`, with a numeric `_2`, `_3`, ... suffix appended if that name is
  *also* taken). Nothing from the left frame is ever overwritten in this mode.
- **`CombineOp::Update`**: only touch features that already exist on this frame —
  `other`-only features are ignored entirely. For each shared feature, `spec.overwrite`
  decides whether `other`'s non-null cells replace the left's values outright
  (`true`, the default), or only fill in the left's own missing cells while leaving
  its existing values alone (`false`).

[`join()`](join.md) and [`update()`](update.md) are both thin convenience wrappers
around `combine()`: `join(other, suffix)` is `combine(other, {.op = Features,
.collision_suffix = suffix})`, and `update(other, overwrite)` is `combine(other, {.op
= Update, .overwrite = overwrite, .cases = FrameMatchRule::KeepLeft})`.

## Parameters

**other** (`const DataFrame&`)

The right-hand frame to combine with. Its cases are matched to this frame's cases by
case-label value, not physical row position.

**spec** (`const CombineSpec&`, default `CombineSpec{}` via the plain overload)

- **`op`** (`CombineOp`, default `Features`): `Features` or `Update`, as described
  above.
- **`collision_suffix`** (`std::string`, default `"_right"`): suffix appended to a
  colliding feature name in `Features` mode. Must be non-empty if any collision
  actually occurs, or the call throws `"DataFrame::combine(features): collision_suffix
  must be non-empty when duplicate features exist"`.
- **`overwrite`** (`bool`, default `true`): in `Update` mode, whether `other`'s
  non-null values replace existing left values (`true`) or only fill left's own nulls
  (`false`).
- **`cases`** (`FrameMatchRule`, default `Unite`): how the two frames' case axes are
  reconciled — `Unite` (union of both sides' case labels), `Common` (only labels
  present on both sides), `KeepLeft` (exactly this frame's case labels), or
  `KeepRight` (exactly `other`'s case labels).

## Returns

`DataFrame`
    The combined frame, following the case-matching rule and feature policy above.

## Examples

Example 1. Default `Features` mode: bring in a right-hand frame's features, with a
name collision disambiguated by suffix, and case labels united across both sides.

```cpp
using namespace pluss::table;

auto left = DataFrame({
    {"name", Slate(pluss::make_note({"Ama", "Kofi", "Efua"}))},
    {"age",  Slate(pluss::make_series({30.0, std::numeric_limits<double>::quiet_NaN(), 28.0}))},
}, Slate(pluss::make_note({"c1", "c2", "c3"})));

auto right = DataFrame({
    {"age",   Slate(pluss::make_series({26.0, 40.0, 22.0}))},
    {"score", Slate(pluss::make_series({88.0, 91.0, 75.0}))},
}, Slate(pluss::make_note({"c2", "c3", "c4"})));

auto out = left.combine(right);
std::cout << out;
```

### Output

```text
===================================
     | name  age   age_right  score
-----------------------------------
c1   | Ama   30    NULL       NULL 
c2   | Kofi  NULL  26         88   
c3   | Efua  28    40         91   
c4   | NULL  NULL  22         75   
-----------------------------------
[4 cs x 4 feat]
```

The default `cases = Unite` produces the union `c1, c2, c3, c4`. `left`'s `name` and
`age` are carried over as-is (`c4` is brand-new, so both are `NULL` there). `right`'s
`age` collides with `left`'s own `age`, so it lands as `age_right` instead of
overwriting anything; `right`'s `score` is entirely new, so it's added under its own
name. `c1` is missing from `right` entirely, so `age_right` and `score` are both
`NULL` there.

Example 2. `Update` mode via the [`update()`](update.md) convenience wrapper:
overwrite matching feature values, keep only the left frame's own cases, and ignore
any right-only feature.

```cpp
auto out = left.update(right);
std::cout << out;
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

`update()` forces `cases = KeepLeft`, so only `c1`, `c2`, `c3` survive — `c4` (present
only in `right`) never appears. `right`'s `score` is ignored entirely, since `Update`
mode never introduces new features. For `age`: `c1` is missing from `right`, so the
left's original `30` is kept; `c2` and `c3` both have a non-null value in `right`
(`26`, `40`), so — with the default `overwrite = true` — they replace the left's
values outright, even overwriting `c2`'s already-non-null-looking gap and `c3`'s
already-present `28`.

Example 3. `Update` mode with `overwrite = false`: only fill the left frame's own
gaps, leaving its already-present values untouched, and keep the default
`cases = Unite` so a right-only case still appears (with `name` left `NULL` there,
since `Update` never introduces a right-only feature to populate it).

```cpp
auto filled = left.combine(right, DataFrame::CombineSpec{
    .op = DataFrame::CombineOp::Update,
    .overwrite = false,
});
std::cout << filled;
```

### Output

```text
=================
     | name  age 
-----------------
c1   | Ama   30  
c2   | Kofi  26  
c3   | Efua  28  
c4   | NULL  22  
-----------------
[4 cs x 2 feat]
```

Compare this to Example 2's `update()` (which forces `overwrite = true` and
`cases = KeepLeft`): here `c4` survives because `cases` stayed at the default
`Unite`, and `c2`'s `age` still ends up `26` — not because `overwrite` replaced it,
but because the left's own `age` was already `NULL` there, so `overwrite = false`'s
"fill gaps only" rule still lets `right`'s value through. `c3`'s left-side `age`
(`28`) is non-null, so it survives untouched instead of being replaced by `right`'s
`40` — the one cell where `overwrite = false` visibly diverges from Example 2's
`overwrite = true` behavior.

Example 4. An empty `collision_suffix` with a genuine feature-name collision
throws instead of silently overwriting or dropping either side's column.

```cpp
try {
  auto out = left.combine(right, DataFrame::CombineSpec{.collision_suffix = ""});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrame::combine(features): collision_suffix must be non-empty when duplicate features exist
```

`left` and `right` both have an `age` feature, so the default `Features` mode
needs to disambiguate one of them with `collision_suffix` — leaving it empty
removes any way to do that safely, so the call rejects it up front rather than
guessing.

## Notes

- Case matching is always by label value, never by physical row position — a `left`
  row and a `right` row line up because they share a case label, not because they
  occupy the same index.
- `Features` mode never overwrites anything from the left frame; `Update` mode never
  introduces a feature the left frame didn't already have. Pick the mode based on
  which invariant you need.
- Setting `overwrite = false` in `Update` mode is a "fill gaps only" merge: only cells
  that are already `NULL` on the left get replaced by `other`'s value, and only if
  `other`'s value there is itself non-null.
- A feature-name collision in `Features` mode with an empty `collision_suffix` throws
  rather than silently dropping or overwriting either side's column.

## See Also

- [`DataFrame::join`](join.md): the `Features`-mode convenience wrapper with a
  custom collision suffix.
- [`DataFrame::update`](update.md): the `Update`-mode, `KeepLeft`-cases convenience
  wrapper shown in Example 2.
- [`DataFrame::align`](align.md): the lower-level case-matching primitive `combine()`
  builds on.
- [`DataFrame::conform`](conform.md): reshape a frame's structure to match another's
  before combining, when the two don't already line up.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clip.html">
    <span class="label">Previous</span>
    <span class="title">clip</span>
  </a>
  <a class="next-link" href="compare.html">
    <span class="label">Next</span>
    <span class="title">compare</span>
  </a>
</div>
