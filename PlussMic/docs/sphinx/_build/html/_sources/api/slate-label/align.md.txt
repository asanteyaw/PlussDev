<!-- pluss-hand-authored -->

# SlateLabelAccessor::align

[Back to SlateLabelAccessor API Reference](../../slate_label_accessor_api_reference)

## Signature

```cpp
SlateLabelAccessor::align(const Slate& target_labels, const AlignOptions& options = {}) const -> Slate
```

## What It Does

A direct forward to `Slate::align(target_labels, options)`. What it actually
does depends entirely on `options.align`:

```cpp
enum class AlignBy : std::uint8_t { Pos = 0, Caselbl = 1, Feature = 2, Both = 3 };
```

- **`AlignBy::Pos`** (the default): a pure **length check** — throws unless
  `target_labels.size() == this->size()`, then returns `*this` completely
  unchanged (case labels included). No reindexing happens at all.
- **`AlignBy::Caselbl`**: a genuine **label-value reindex** — every row of the
  result is matched to a row of `*this` whose current case label equals the
  corresponding entry of `target_labels`, by value, occurrence-by-occurrence
  (first target occurrence to first matching source occurrence, second to
  second, and so on). A target occurrence beyond the number of matching
  source rows — or a label absent from the source case labels entirely —
  produces `NULL` in the result (or `options.fill`, for numeric `Series`
  slates only). The result's case labels become `target_labels` exactly.
- **`AlignBy::Feature`** / **`AlignBy::Both`**: not meaningful for a single
  `Slate` (a `Slate` has no features to align by); `Both` throws
  unconditionally, and `Feature` is only sensible for `DataFrame`-to-`DataFrame`
  alignment.

```{important} The default (Pos) does not reindex by label value
It is easy to assume `.label().align(target)` behaves like
`DataFrameLabelAccessor::align`, which always reindexes by value. It does not,
unless you explicitly pass `AlignOptions{.align = AlignBy::Caselbl}`. With the
default `options`, `align()` is only a length assertion — see Example 1 vs.
Example 2.
```

## Parameters

**target_labels** (`const Slate&`)

Under `AlignBy::Pos`, only its length matters. Under `AlignBy::Caselbl`, its
values (including repeats) become the reindex target and the result's own new
case labels.

**options** (`const AlignOptions&`)

`align` (default `AlignBy::Pos`) selects the behavior described above. `fill`
(`std::optional<double>`, default `std::nullopt`) supplies a constant for
unmatched cells under `AlignBy::Caselbl`, but only reaches numeric (`Series`)
slates — a `Note`/`Timeline`/`TimeIndex` slate's unmatched cells stay `NULL`
regardless.

## Returns

`Slate`
    Under `Pos`: `*this`, unchanged. Under `Caselbl`: a new slate reindexed
    onto `target_labels`.

## Examples

Example 1. The default `AlignBy::Pos` — a length check only, no reindexing.

```cpp
using namespace pluss::table;

auto tagged = Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))
                  .label().set(Slate(pluss::make_note({"east", "west", "east", "north"})));

auto same_length = Slate(pluss::make_note({"a", "b", "c", "d"}));
auto out = tagged.label().align(same_length);
std::cout << out;
```

### Output

```text
east   100
west   200
east   150
north  90

ptype: Series
```

`out` is identical to `tagged` — the case labels are still `"east"`/`"west"`/
`"east"`/`"north"`, completely unaffected by `same_length`'s own values, since
`AlignBy::Pos` only checked that the lengths matched (`4 == 4`).

Example 2. `AlignBy::Caselbl` — a genuine label-value reindex, with a repeat
and one label absent from the source.

```cpp
AlignOptions by_label;
by_label.align = AlignBy::Caselbl;

auto target = Slate(pluss::make_note({"west", "east", "south"}));
auto out2 = tagged.label().align(target, by_label);
std::cout << out2;
```

### Output

```text
west   200
east   100
south  NULL

ptype: Series
```

`"west"` and `"east"` each match their first occurrence in `tagged`'s case
labels; `"south"` never appears, so its cell is `NULL`.

Example 3. The same reindex with a numeric `fill` for unmatched cells.

```cpp
AlignOptions with_fill;
with_fill.align = AlignBy::Caselbl;
with_fill.fill = 0.0;

auto out3 = tagged.label().align(target, with_fill);
std::cout << out3;
```

### Output

```text
west   200
east   100
south  0

ptype: Series
```

Example 4. A length mismatch under the default `AlignBy::Pos` throws.

```cpp
auto wrong_length = Slate(pluss::make_note({"a", "b"}));
try {
  auto bad = tagged.label().align(wrong_length);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::align: positional alignment requires identical lengths
```

## Notes

- The default `options.align = AlignBy::Pos` does **not** reindex by label
  value — see the important callout above.
- `AlignBy::Caselbl` matching is purely by label **value**, occurrence-by-
  occurrence — not by source row position or sort order.
- `fill` never reaches non-numeric slates; an unmatched `Note`/`Timeline`/
  `TimeIndex` cell is always `NULL` regardless of `fill`.
- `AlignBy::Both` always throws for `Slate`-level alignment (it's only valid
  DataFrame-to-DataFrame); `AlignBy::Feature` has no meaningful effect on a
  single `Slate`.

## See Also

- [`Slate::align`](../slate/align.md): the direct, non-accessor form this
  method forwards to.
- [`SlateLabelAccessor::set`](set.md): attach the case labels that `align()`
  reindexes against.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="diff.html">
    <span class="label">Next</span>
    <span class="title">diff</span>
  </a>
</div>
