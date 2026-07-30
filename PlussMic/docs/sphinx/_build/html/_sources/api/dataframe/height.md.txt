<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9857b32db98d -->

# DataFrame::height

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::height() const -> std::int64_t
```

## What It Does

A convenience alias for [`ncases()`](ncases.md): the number of cases (rows) in the
frame. `height()` and `width()` exist as a pair, reading naturally when you think of
a frame as a grid — `height()` down, [`width()`](width.md) across — the same
convention used by tools like Polars, as an alternative to the `ncases()`/
`nfeatures()` naming for people coming from that background. There is no independent
computation here: `height()` returns exactly what `ncases()` returns.

## Parameters

`None`

## Returns

`std::int64_t`
    The number of cases in the frame — identical to `ncases()`, and to `shape().first`.

## Examples

Example 1. Read the row count via `height()`.

```cpp
using namespace pluss::table;

auto scores = DataFrame({
    {"quiz",  pluss::make_series({84.0, 90.0, 84.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = scores.height();
std::cout << out;
```

### Output

```text
3
```

Same value `ncases()` would give, and the same `3` that appears in `scores`'s own
`[3 cs x 2 feat]` footer when printed.

## Notes

- `height() == ncases() == shape().first` always — pick whichever name reads better
  at the call site.
- Like `ncases()`, `height()` counts rows, not distinct case labels.

## See Also

- [`DataFrame::width`](width.md): the companion alias for `nfeatures()`.
- [`DataFrame::ncases`](ncases.md): the underlying accessor `height()` delegates to.
- [`DataFrame::shape`](shape.md): both counts together as one pair.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="head.html">
    <span class="label">Previous</span>
    <span class="title">head</span>
  </a>
  <a class="next-link" href="info.html">
    <span class="label">Next</span>
    <span class="title">info</span>
  </a>
</div>
