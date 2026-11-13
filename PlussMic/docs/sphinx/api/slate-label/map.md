<!-- pluss-hand-authored -->

# SlateLabelAccessor::map

[Back to SlateLabelAccessor API Reference](../../slate_label_accessor_api_reference)

## Signature

```cpp
Slate SlateLabelAccessor::map(const std::function<SlateValue(const SlateValue&)>& fn) const
```

## What It Does

Applies `fn` to every case label individually and reattaches the results as
the bound slate's new case labels — a direct forward to
`Slate::caselbl(Slate::caselbl().map(fn))`. The slate's own values are
untouched; only the case-label companion sequence changes.

```{note} Always reachable — unlike DataFrameLabelAccessor::map
`DataFrameLabelAccessor::map` throws on a structured (multi-level) frame,
since there's no single flat slate to map over. A `Slate`'s case labels
are always a single flat sequence, so `SlateLabelAccessor::map` never has
that concern.
```

## Parameters

**fn** (`const std::function<SlateValue(const SlateValue&)>&`)

A callback receiving one case label at a time (wrapped in a `SlateValue`)
and returning its replacement.

## Returns

`Slate`
    A new slate, holding the same values as `*this`, with case labels
    replaced by `fn`'s output.

## Examples

Example 1. Append a suffix to every case label.

```cpp
using namespace pluss::table;

auto tagged = Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))
                  .label().set(Slate(pluss::make_note({"r1", "r2", "r3", "r4"})));
std::cout << tagged;

auto out = tagged.label().map([](const SlateValue& v) {
  return SlateValue(Slate(pluss::make_note({v.to_str() + "!"})));
});
std::cout << out;
```

### Output (before)

```text
r1  100
r2  200
r3  150
r4  90

ptype: Series
```

### Output (after)

```text
r1!  100
r2!  200
r3!  150
r4!  90

ptype: Series
```

Each case label is transformed by appending `"!"`; the slate's own `sales`-
style values are exactly as they were.

## Notes

- `fn` must return a `SlateValue` of a kind consistent with the rest of the
  output (a length-1 `Slate` wrapped in `SlateValue`, as constructed above).
- Only the case labels change — the bound slate's own values are untouched.

## See Also

- [`SlateLabelAccessor::values`](values.md): pull the raw label slate if a
  transform outside `map(...)` is needed.
- [`SlateLabelAccessor::set`](set.md): the underlying mechanism this method
  applies its result through.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnull.html">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
  </a>
  <a class="next-link" href="monotonic.html">
    <span class="label">Next</span>
    <span class="title">monotonic</span>
  </a>
</div>
