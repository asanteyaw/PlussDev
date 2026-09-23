<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 299768aa1d16 -->

# Atlas::dloc

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::dloc(std::int64_t index) const -> DataFrame
```

## What It Does

An exact synonym for [`frame()`](frame.md) — `dloc()` forwards directly to `frame()`
with no behavior difference whatsoever, including negative-index support. It exists for
call sites that want the depth-axis wording to be explicit (`dloc`, echoing `.loc`-style
label access elsewhere in the codebase), the way `frame()` reads more naturally in
others.

## Parameters

**index** (`std::int64_t`)

Zero-based position in the stack. Negative values count back from the end
(`-1` is `depth() - 1`). Out-of-range values throw.

## Returns

`DataFrame`
    The frame at the requested depth position — identical to what `frame(index)` would
    return for the same `index`.

## Examples

Example 1. `dloc()` and `frame()` return the same result for the same index.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"sales", Thread(pluss::make_series({100.0, 120.0}))}});
auto q2 = DataFrame({{"sales", Thread(pluss::make_series({110.0, 90.0}))}});
auto atlas = Atlas({q1, q2});

std::cout << atlas.dloc(1);
```

### Output

```text
==========
     | sales
----------
0    | 110
1    | 90
----------
[2 cs x 1 feat]
```

## Notes

- `dloc(i)` and `frame(i)` are the same call under two names — pick whichever reads
  better at the call site.
- Requires a defined `Atlas`; throws `"Atlas: undefined handle"` on a
  default-constructed `Atlas()`.

## See Also

- [`Atlas::frame`](frame.md): the exact same operation, under a second name.
- [`Atlas::loc`](loc.md): retrieve a frame by its depth **label** instead of its
  positional index.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="depth.html">
    <span class="label">Previous</span>
    <span class="title">depth</span>
  </a>
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>
