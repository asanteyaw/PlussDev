<!-- pluss-hand-authored -->

# Thread::ncases

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::ncases() const -> std::int64_t
```

## What It Does

An exact alias for [`size()`](size.md) — returns the same element count, under the
"cases" terminology used consistently across `DataFrame::ncases()`. There is no
behavioral difference between the two; `ncases()` exists purely so code working with
both `Thread` and `DataFrame` can use the same method name for "how many rows."

## Parameters

`None`

## Returns

`std::int64_t`
    The element count — identical to `size()`.

## Examples

Example 1. `ncases()` and `size()` always agree.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
std::cout << scores.ncases() << " " << scores.size();
```

### Output

```text
4 4
```

## Notes

- Literally `return size();` internally — no separate logic.
- Exists for naming symmetry with `DataFrame::ncases()`.

## See Also

- [`Thread::size`](size.md): the method `ncases()` delegates to.
- [`Thread::shape`](shape.md): the pair form, `{ncases(), 1}`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mul.html">
    <span class="label">Previous</span>
    <span class="title">mul</span>
  </a>
  <a class="next-link" href="ndistinct.html">
    <span class="label">Next</span>
    <span class="title">ndistinct</span>
  </a>
</div>
