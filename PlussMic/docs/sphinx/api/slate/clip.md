<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a2c3dd572e4f -->

# Slate::clip

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::clip(double min_value, double max_value) const -> Slate
```

## What It Does

Clamps every element into the closed interval `[min_value, max_value]` — a value
below `min_value` becomes `min_value`, a value above `max_value` becomes
`max_value`, and everything in between is left untouched. Requires the slate to be
`Series`-kind.

## Parameters

**min_value** (`double`)
    The lower bound. Values below this are raised to it.

**max_value** (`double`)
    The upper bound. Values above this are lowered to it.

## Returns

`Slate`
    A new `Series`-kind slate, same length as `*this`, with every value clamped
    into `[min_value, max_value]`.

## Examples

Example 1. Values outside the bounds are pulled to the nearest bound; values already
inside are untouched.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto out = scores.clip(15.0, 35.0);
std::cout << out;
```

### Output

```text
0  15
1  20
2  30
3  35

ptype: Series
```

`10.0` (below `15.0`) is raised to `15.0`; `40.0` (above `35.0`) is lowered to
`35.0`; `20.0` and `30.0`, already inside `[15.0, 35.0]`, are unchanged.

Example 2. A non-`Series` slate throws.

```cpp
auto notes = Slate(pluss::make_note({"a", "b"}));
try {
  auto bad = notes.clip(0.0, 1.0);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::clip is only valid for Series payload
```

## Notes

- Requires the calling slate to be `Series`-kind.
- Both bounds are inclusive.
- A `NULL` cell on input stays `NULL` on output.

## See Also

- [`Slate::abs`](abs.md): a different sign/magnitude transform, not a range clamp.
- [`Slate::between`](between.md): test membership in a range, rather than clamping into it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="chars.html">
    <span class="label">Previous</span>
    <span class="title">chars</span>
  </a>
  <a class="next-link" href="compare.html">
    <span class="label">Next</span>
    <span class="title">compare</span>
  </a>
</div>
