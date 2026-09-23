<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5b1f486bcd74 -->

# DataFrame::width

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::width() const -> std::int64_t
```

## What It Does

A convenience alias for [`nfeatures()`](nfeatures.md): the number of features
(columns) in the frame. Paired with [`height()`](height.md), it reads naturally
when you think of a frame as a grid — `height()` down, `width()` across. There is
no independent computation here: `width()` returns exactly what `nfeatures()`
returns.

## Parameters

`None`

## Returns

`std::int64_t`
    The number of features in the frame — identical to `nfeatures()`, and to
    `shape().second`.

## Examples

Example 1. Read the column count via `width()`.

```cpp
using namespace pluss::table;

auto scores = DataFrame({
    {"quiz",  pluss::make_series({84.0, 90.0, 84.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = scores.width();
std::cout << out;
```

### Output

```text
2
```

Same value `nfeatures()` would give, and the same `2` that appears in `scores`'s own
`[3 cs x 2 feat]` footer when printed.

## Notes

- `width() == nfeatures() == shape().second` always — pick whichever name reads
  better at the call site.
- [`DataFrame::length`](length.md) builds directly on this: `length() == width() *
  height()`, the total cell count.

## See Also

- [`DataFrame::height`](height.md): the companion alias for `ncases()`.
- [`DataFrame::nfeatures`](nfeatures.md): the underlying accessor `width()` delegates to.
- [`DataFrame::length`](length.md): the total cell count, `width() * height()`.
- [`DataFrame::shape`](shape.md): both counts together as one pair.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="where.html">
    <span class="label">Previous</span>
    <span class="title">where</span>
  </a>
</div>
