<!-- pluss-hand-authored -->

# Thread::isnan

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::isnan() const -> Thread
```

## What It Does

Returns a `Bool`-typed `Series` thread, same length as `*this`, marking `true`
wherever the underlying floating-point value is `NaN`. Requires the thread to be
`Series`-kind — checks the actual numeric value, unlike [`isnull()`](isnull.md),
which only looks at the validity mask and doesn't inspect the value at all.

## Parameters

`None`

## Returns

`Thread`
    A new `Bool`-typed `Series` thread, same length as `*this`.

## Examples

Example 1. `isnan()` flags `NaN` values regardless of the validity mask.

```cpp
using namespace pluss::table;

auto readings = Thread(pluss::make_series({1.0, std::nan(""), 3.0}));
std::cout << readings.isnan();
```

### Output

```text
0  false
1  true
2  false

ptype: Series
```

Example 2. A non-`Series` thread throws.

```cpp
auto notes = Thread(pluss::make_note({"a", "b"}));
try {
  auto bad = notes.isnan();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::isnan is only valid for Series payload
```

## Notes

- Requires the calling thread to be `Series`-kind.
- Checks the actual floating-point value — unaffected by, and independent of, the
  validity mask that [`isnull()`](isnull.md) checks.
- A cell can be `NaN` and still report `false` from `isnull()`, and vice versa (a
  masked-`NULL` non-`NaN` value reports `false` from `isnan()`).

## See Also

- [`Thread::isnull`](isnull.md): the validity-mask check, as opposed to this
  method's numeric-value check.
- [`Thread::isfinite`](isfinite.md): the complementary "is this both non-`NaN` and
  non-`Inf`, and also valid" check.
- [`Thread::invalid`](invalid.md): the union of `NULL`, `NaN`, and `Inf` in one
  call.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isfinite.html">
    <span class="label">Previous</span>
    <span class="title">isfinite</span>
  </a>
  <a class="next-link" href="isnull.html">
    <span class="label">Next</span>
    <span class="title">isnull</span>
  </a>
</div>
