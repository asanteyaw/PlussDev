<!-- pluss-hand-authored -->

# DataFrame::extend

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::extend(const DataFrame& other, FrameMatchRule features = FrameMatchRule::Unite, bool fresh_cases = false, bool reject_repeated_cases = false) const -> DataFrame
```

## What It Does

Appends `other`'s cases beneath this frame's cases — a vertical "stack more rows on"
operation, as opposed to [`combine()`](combine.md), which merges two frames'
*features* side by side. The two frames' feature sets are reconciled according to
`features` before stacking: features unique to one side are either kept (padded with
`NULL` for the side that doesn't have them) or dropped, depending on the rule. The
resulting case-label axis is, by default, simply this frame's case labels followed by
`other`'s — unless `fresh_cases` asks for a brand-new `0..N-1` index instead.

## Parameters

**other** (`const DataFrame&`)

The frame whose cases are appended beneath this frame's.

**features** (`FrameMatchRule`, default `Unite`)

How the two frames' *feature* sets are reconciled before stacking:

- **`Unite`** (default): keep this frame's feature order, then append any
  `other`-only features after it. A feature missing from one side is `NULL` for every
  row that came from that side.
- **`Common`**: keep only features present, by name, on both sides — a feature unique
  to either frame is dropped from the result entirely.
- **`KeepLeft`**: keep exactly this frame's feature set, in this frame's order,
  discarding anything `other`-only.
- **`KeepRight`**: keep exactly `other`'s feature set, in `other`'s order, discarding
  anything unique to this frame.

**fresh_cases** (`bool`, default `false`)

- `false` (default): the output case axis is this frame's case labels followed by
  `other`'s, unchanged.
- `true`: discard both sides' original case labels and assign a fresh, contiguous
  `0, 1, 2, ..., ncases()+other.ncases()-1` index instead.

**reject_repeated_cases** (`bool`, default `false`)

When `true` (and `fresh_cases` is `false`), throws if the concatenated case-label axis
would contain any label more than once — including a label that appears twice within
one side alone, or a label shared by both sides. The exact message is
`"DataFrame::match: repeated cases on the combined frame require
RepeatedCaseRule::PairInOrder"`.

## Returns

`DataFrame`
    A new frame with `other`'s cases stacked beneath this frame's, and features
    reconciled per `features`.

## Examples

Example 1. Stack two frames with the default `Unite` feature rule, where one side has
an extra feature the other doesn't.

```cpp
using namespace pluss::table;

auto jan = DataFrame({
    {"id",     Thread(pluss::make_series({1, 2}))},
    {"amount", Thread(pluss::make_series({100.0, 150.0}))},
}, Thread(pluss::make_note({"a", "b"})));

auto feb = DataFrame({
    {"id",     Thread(pluss::make_series({3, 4}))},
    {"amount", Thread(pluss::make_series({200.0, 50.0}))},
    {"bonus",  Thread(pluss::make_series({10.0, 20.0}))},
}, Thread(pluss::make_note({"c", "d"})));

auto out = jan.extend(feb);
std::cout << out;
```

### Output

```text
==========================
     | id    amount  bonus
--------------------------
a    | 1     100     NULL 
b    | 2     150     NULL 
c    | 3     200     10   
d    | 4     50      20   
--------------------------
[4 cs x 3 feat]
```

`bonus` only exists on `feb`, so it's kept (per `Unite`) and `jan`'s two rows get
`NULL` there. Case labels are simply `jan`'s (`a, b`) followed by `feb`'s (`c, d`).

Example 2. Discard the original case labels and get a fresh contiguous index instead.

```cpp
auto out = jan.extend(feb, FrameMatchRule::Unite, /*fresh_cases=*/true);
std::cout << out;
```

### Output

```text
==========================
     | id    amount  bonus
--------------------------
0    | 1     100     NULL 
1    | 2     150     NULL 
2    | 3     200     10   
3    | 4     50      20   
--------------------------
[4 cs x 3 feat]
```

Same values and row order as Example 1 — only the case-label axis changed, from
`a, b, c, d` to a plain `0, 1, 2, 3` index. This is the same "give me a fresh index"
idiom useful when the original labels (from either side) no longer carry meaningful
information after stacking.

Example 3. Use `Common` to drop a feature unique to one side entirely, rather than
padding it with `NULL`.

```cpp
auto out = jan.extend(feb, FrameMatchRule::Common);
std::cout << out;
```

### Output

```text
===================
     | id    amount
-------------------
a    | 1     100   
b    | 2     150   
c    | 3     200   
d    | 4     50    
-------------------
[4 cs x 2 feat]
```

`bonus` is gone from the result entirely — `Common` keeps only features present on
both sides, unlike `Unite`'s pad-with-`NULL` behavior in Example 1.

## Notes

- `extend()` stacks rows; it never merges two frames' features into new combined
  columns the way [`combine()`](combine.md) does. Use `extend()` when you have "more
  of the same kind of row" to append, and `combine()` when you have "more columns
  about the same rows" to bring in.
- `reject_repeated_cases` only has an effect when `fresh_cases` is `false` — a fresh
  index can never contain a repeated label, so there's nothing to reject in that case.
- A duplicate case label across (or within) the two frames is otherwise allowed by
  default (`reject_repeated_cases = false`); set it to `true` only when you need a
  guaranteed-unique case axis on the result and would rather fail loudly than silently
  produce one with repeats.
- [`head(0)`](head.md) is a common way to get an empty-but-typed seed frame — same
  features, zero cases — to repeatedly `extend()` inside a loop.

## See Also

- [`DataFrame::combine`](combine.md): merge two frames' *features* together instead
  of stacking their cases.
- [`DataFrame::head`](head.md): often used to build an empty, correctly-typed seed
  frame before a loop of repeated `extend()` calls.
- [`DataFrame::conform`](conform.md): reshape a frame's structure to match another's,
  when the two don't already share a compatible feature layout.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="exp.html">
    <span class="label">Previous</span>
    <span class="title">exp</span>
  </a>
  <a class="next-link" href="factor.html">
    <span class="label">Next</span>
    <span class="title">factor</span>
  </a>
</div>
