<!-- pluss-hand-authored -->

# Thread::isfinite

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::isfinite() const -> Thread
```

## What It Does

Returns a `Bool`-typed `Series` thread, same length as `*this`, marking `true`
wherever the value is both a real finite number (not `NaN`, not `±Inf`) **and**
valid (not `NULL`). Requires the thread to be `Series`-kind. Unlike
[`isnan()`](isnan.md), which checks the numeric value alone, `isfinite()` combines
the numeric check with the validity mask — a masked-`NULL` cell is `false` here even
if its underlying stored value happens to be an ordinary finite number.

## Parameters

`None`

## Returns

`Thread`
    A new `Bool`-typed `Series` thread, same length as `*this`.

## Examples

Example 1. `isfinite()` is `false` for `NaN`, `Inf`, and masked-`NULL` cells alike —
only a genuinely valid, finite value is `true`.

```cpp
using namespace pluss::table;

auto readings = Thread(
    pluss::make_series({1.0, std::nan(""), std::numeric_limits<double>::infinity(), 4.0}),
    pluss::make_series(std::vector<bool>{true, true, true, false}));
std::cout << readings.isfinite();
```

### Output

```text
0  true
1  false
2  false
3  false

ptype: Series
```

Position `1` (`NaN`) and position `2` (`Inf`) are both `false` because they aren't
finite numbers; position `3` is `false` because it's masked `NULL`, even though its
underlying stored value (`4.0`) is an ordinary finite number. Only position `0` is
genuinely finite *and* valid.

Example 2. A non-`Series` thread throws.

```cpp
auto notes = Thread(pluss::make_note({"a", "b"}));
try {
  auto bad = notes.isfinite();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::isfinite is only valid for Series payload
```

## Notes

- Requires the calling thread to be `Series`-kind.
- Combines a numeric finiteness check with the validity mask — `false` for `NaN`,
  `±Inf`, *and* masked-`NULL` cells.
- The exact opposite question `NaN`-only checks like [`isnan()`](isnan.md) answer:
  `isfinite()` folds in validity too, `isnan()` doesn't.

## See Also

- [`Thread::isnan`](isnan.md): the numeric-value-only check, without folding in the
  validity mask.
- [`Thread::isnull`](isnull.md): the validity-mask-only check, without inspecting
  the numeric value.
- [`Thread::invalid`](invalid.md): the logical inverse of this method (for
  `Series`-kind threads).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_valid.html">
    <span class="label">Previous</span>
    <span class="title">is_valid</span>
  </a>
  <a class="next-link" href="isnan.html">
    <span class="label">Next</span>
    <span class="title">isnan</span>
  </a>
</div>
