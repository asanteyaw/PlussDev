<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 53d611e2cec4 -->

# Slate::rank

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate Slate::rank(RankTieRule ties = RankTieRule::Average, RankNullRule nulls = RankNullRule::Keep, bool ascending = true, bool percentage = false) const
```

## What It Does

Assigns a numeric rank (`1` = smallest, by default) to every entry, based on
its value relative to the rest of the slate. Ties — entries with equal
value — are resolved by `ties`, and missing entries are handled by `nulls`.
Always returns a `Float64` (or `Float32` off-CPU) slate the same length as the
source, regardless of the source's own dtype, since `Average` and `percentage`
ranks are inherently fractional. Restricted to numeric `Series` payloads; every
other kind or dtype throws.

`ties` controls what rank a group of equal values receives:

- **`Average`** (the default): every tied entry gets the mean of the rank
  positions the group spans (e.g. a 2-way tie for ranks `3`–`4` gives both
  entries `3.5`).
- **`Lowest`**: every tied entry gets the group's lowest rank position.
- **`Highest`**: every tied entry gets the group's highest rank position.
- **`First`**: ties are broken by original order — the earlier-occurring entry
  gets the lower rank number, so no two entries ever tie.
- **`Dense`**: like `Lowest`, but the next distinct value's rank always follows
  immediately after (no rank numbers are skipped for the size of a tie group).

`nulls` controls what happens to missing entries:

- **`Keep`** (the default): missing entries are excluded from ranking
  entirely, and their own output position is missing too.
- **`Top`**: missing entries participate in ranking and are treated as the
  smallest (or largest, if `ascending = false`) possible value, so they receive
  the lowest ranks. Every position gets a real (non-missing) rank.
- **`Bottom`**: the mirror of `Top` — missing entries rank last. Every
  position gets a real rank.

## Parameters

**ties** (`RankTieRule`, default `Average`)
    How equal-valued entries share rank numbers: `Average`, `Lowest`,
    `Highest`, `First`, or `Dense`.

**nulls** (`RankNullRule`, default `Keep`)
    How missing entries are handled: `Keep` (exclude, stays missing), `Top`
    (ranks first), or `Bottom` (ranks last).

**ascending** (`bool`, default `true`)
    `true` ranks the smallest value `1`; `false` reverses it so the largest
    value is `1`.

**percentage** (`bool`, default `false`)
    When `true`, divides every rank by the number of ranked entries, producing
    a fraction in `(0, 1]` instead of a raw rank number.

## Returns

`Slate`
    A `Float64` (or `Float32` off-CPU) slate of ranks, the same length as the
    source.

## Examples

Example 1. Default ranking — smallest value gets rank `1`, ties average their
rank positions.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({10.0, 10.0, 20.0, 20.0}));
auto out = scores.rank();
std::cout << out;
```

### Output

```text
0  1.5
1  1.5
2  3.5
3  3.5

ptype: Series
```

Both `10.0`s share ranks `1`–`2` (averaging to `1.5`); both `20.0`s share ranks
`3`–`4` (averaging to `3.5`).

Example 2. `ties = RankTieRule::First` breaks ties by original order instead
of averaging — no two entries ever tie.

```cpp
auto out2 = scores.rank(RankTieRule::First);
std::cout << out2;
```

### Output

```text
0  1
1  2
2  3
3  4

ptype: Series
```

The first `10.0` (position `0`) gets rank `1`; the second (position `1`) gets
`2` — purely by which one appeared first in the slate.

Example 3. `ties = RankTieRule::Dense` doesn't skip rank numbers for the size
of a tie group.

```cpp
auto out3 = scores.rank(RankTieRule::Dense);
std::cout << out3;
```

### Output

```text
0  1
1  1
2  2
3  2

ptype: Series
```

Compare to Example 1: `Average` jumped from the `10.0` group (ranks `1`–`2`)
straight to `3.5` for the `20.0` group; `Dense` instead moves to rank `2`
immediately, since it's simply "the next distinct value" rather than "the next
unused rank position."

Example 4. `nulls = RankNullRule::Keep` (the default) excludes missing entries
from ranking — they stay missing in the output too.

```cpp
auto with_gap = Slate(
    pluss::make_series({10.0, 20.0, 30.0}),
    pluss::make_series(std::vector<bool>{true, false, true})
);
auto out4 = with_gap.rank();
std::cout << out4;
```

### Output

```text
0  1
1  NULL
2  2

ptype: Series
```

Example 5. `nulls = RankNullRule::Top` gives the missing entry a real rank —
the lowest one, since it's treated as smaller than every observed value.

```cpp
auto out5 = with_gap.rank(RankTieRule::Average, RankNullRule::Top);
std::cout << out5;
```

### Output

```text
0  2
1  1
2  3

ptype: Series
```

Every position now has a real rank — position `1` (the missing entry) ranks
`1`, ahead of both observed values.

Example 6. `percentage = true` divides every rank by the total ranked count.

```cpp
auto clean = Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto out6 = clean.rank(RankTieRule::Average, RankNullRule::Keep, true, true);
std::cout << out6;
```

### Output

```text
0  0.25
1  0.5
2  0.75
3  1

ptype: Series
```

## Notes

- Always returns `Float64`/`Float32`, regardless of the source dtype — `1.0`
  through `size()` (or a fraction of that range with `percentage = true`), not
  necessarily whole numbers once `ties = Average` or `percentage = true` is in
  play.
- `nulls = Keep` is the only mode where the output can itself contain missing
  entries; `Top` and `Bottom` always produce a fully-valid rank slate.
- Restricted to numeric `Series` payloads; anything else throws
  `"Slate::rank: expected a real numeric Series Slate"`.

## See Also

- [`Slate::arrange`](arrange.md): reorder by value directly, rather
  than assigning a rank number in place.
- [`Slate::percentiles`](percentiles.md): the value at a given fractional
  position, the inverse question to ranking.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rand.html">
    <span class="label">Previous</span>
    <span class="title">rand</span>
  </a>
  <a class="next-link" href="rdiv.html">
    <span class="label">Next</span>
    <span class="title">rdiv</span>
  </a>
</div>
