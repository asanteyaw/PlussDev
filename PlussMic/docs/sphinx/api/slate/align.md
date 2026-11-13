<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3142fe9ac3b4 -->

# Slate::align

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate Slate::align(const Slate& target_labels, const AlignOptions& options = {}) const
```

## What It Does

Realigns one slate's values to a target label order — the typed, single-column
alignment surface behind many higher-level `DataFrame` operations. Use it when you're
working with one extracted column directly and need to reorder or relabel it against
another label vector before comparison, arithmetic, export, or manual reconstruction.

### `AlignBy` values

| Value | Numeric value | Description |
|---|---|---|
| `Pos` | `0` | strict positional alignment — no label-based remapping |
| `Caselbl` | `1` | gather by case label |
| `Feature` | `2` | gather by feature label |
| `Both` | `3` | DataFrame-only; rejected for Slate |

### `AlignOptions` fields

| Field | Type | Default |
|---|---|---|
| `align` | `AlignBy` | `AlignBy::Pos` |
| `fill` | `std::optional<double>` | `std::nullopt` |

`target_labels`' interpretation depends entirely on `options.align`: under `Caselbl`, it's
the desired case-axis order; under `Feature`, the desired feature-label order; under `Pos`
(the default), it's only checked for length compatibility — no label matching happens at
all, and the slate's own current values and visible labels pass through unchanged.

```{important} Unmatched target labels become NULL unless fill is set
Under `Caselbl`/`Feature` alignment, a `target_labels` entry with no corresponding source
label produces a `NULL` row in the output — not a thrown error. Set `options.fill` to a
numeric scalar if you'd rather those gaps came back filled instead. See Examples 1 and 2.
```

## Parameters

**target_labels** (`const Slate&`)
    The target label vector defining the output order — typically a `Note`-kind slate
    of label strings (a bare `pluss::Note` also works, thanks to `Slate`'s implicit
    constructor from `Note`). Under `AlignBy::Pos`, only its length matters.

**options** (`const AlignOptions&`)
    `align` selects the matching rule (see above); `fill`, if set, supplies a numeric
    scalar for target labels that have no match in the source slate. `AlignBy::Both` is
    rejected — it's a `DataFrame`-only concept (aligning both the case axis and the
    feature axis at once), which has no meaning for a single `Slate`.

## Returns

`Slate`
    A new slate reordered/relabeled per `options.align`, the same length as
    `target_labels`.

## Examples

Example 1. Align by case label, leaving an unmatched target label `NULL`.

```cpp
using namespace pluss::table;

auto labeled = Slate(pluss::make_series({10.0, 20.0, 30.0}))
                   .label().set(pluss::make_note({"b", "a", "c"}));

auto out = labeled.align(
    pluss::make_note({"a", "b", "d"}),
    AlignOptions{.align = AlignBy::Caselbl}
);
std::cout << out;
```

### Output

```text
a  20
b  10
d  NULL
ptype: Series
```

Example 2. The same case-label alignment, now with `fill` supplying a value for the
unmatched `"d"` label instead of leaving it `NULL`.

```cpp
auto labeled = Slate(pluss::make_series({10.0, 20.0, 30.0}))
                   .label().set(pluss::make_note({"b", "a", "c"}));

auto out = labeled.align(
    pluss::make_note({"a", "b", "d"}),
    AlignOptions{.align = AlignBy::Caselbl, .fill = 0.0}
);
std::cout << out;
```

### Output

```text
a  20
b  10
d  0
ptype: Series
```

Example 3. Align a one-value-per-feature slate by feature label — useful when a slate
represents, e.g., one aggregate per column and you need it in a specific column order.

```cpp
auto feature_slate = Slate(pluss::make_series({1.5, 2.5}))
                          .label().set(pluss::make_note({"A2", "A1"}));

auto out = feature_slate.align(
    pluss::make_note({"A1", "A2"}),
    AlignOptions{.align = AlignBy::Feature}
);
std::cout << out;
```

### Output

```text
A1  2.5
A2  1.5
ptype: Series
```

Example 4. `AlignBy::Pos` performs no label-based remapping at all — it only validates
length compatibility, and the slate's own values and visible labels pass through
unchanged even though `target_labels` here holds entirely different label text.

```cpp
auto labeled = Slate(pluss::make_series({10.0, 20.0, 30.0}))
                   .label().set(pluss::make_note({"b", "a", "c"}));

auto out = labeled.align(
    pluss::make_note({"x", "y", "z"}),
    AlignOptions{.align = AlignBy::Pos}
);
std::cout << out;
```

### Output

```text
b  10
a  20
c  30
ptype: Series
```

Example 5. `AlignBy::Both` is a `DataFrame`-only concept and throws when requested on a
`Slate`.

```cpp
auto slate = Slate(pluss::make_series({10.0, 20.0, 30.0}));
try {
  auto out = slate.align(pluss::make_note({"a", "b", "c"}), AlignOptions{.align = AlignBy::Both});
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::align: AlignBy::Both is only valid for DataFrame-to-DataFrame alignment
```

## Notes

- `AlignBy::Both` is rejected on `Slate` — see Example 5. It only makes sense for
  `DataFrame::align`, which can align both axes (case and feature) simultaneously.
- Unmatched target labels become `NULL` unless `options.fill` is set — see the important
  callout above.
- `AlignBy::Pos` does not broadcast or remap by label at all; it expects a compatible
  length and preserves the slate's current value order and visible labels untouched.

## See Also

- [`DataFrame::align`](../dataframe/align.md): frame-level alignment for several features
  at once, including the `Both`-axis mode this method rejects.
- [`Slate::label`](label.md): use `label().set(...)` to attach explicit case/feature
  labels before aligning by label, as every example above does.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add.html">
    <span class="label">Previous</span>
    <span class="title">add</span>
  </a>
  <a class="next-link" href="all.html">
    <span class="label">Next</span>
    <span class="title">all</span>
  </a>
</div>
