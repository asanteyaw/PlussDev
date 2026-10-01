<!-- pluss-hand-authored -->

# Series::set

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::set_() const
Series& Series::set_(const Series& other) const
```

## What It Does

`set_()` (zero-argument) is a pure no-op — it returns a reference to `self` unmodified, without touching its storage or bumping its version. It exists purely for torch-style API compatibility.

`set_(other)` is the real, meaningful overload: it makes `self` alias `other`'s storage — after the call, `self` holds `other`'s values (converted to `self`'s dtype if the dtypes differ), effectively swapping out which buffer `self` refers to. This is different from an ordinary in-place value op: it can change `self`'s shape as well as its values.

Despite being declared `const` at the call site, `set_(other)` genuinely mutates `self`.

## Parameters

**other** (`const Series&`, optional) — the `Series` whose storage `self` adopts. Omitted, the call is a no-op.

## Returns

`Series&` — a reference to `self`. After `set_(other)`, holding `other`'s values (and, in general, `other`'s shape).

## Examples

Example 1. `set_(other)` replaces `self`'s values (and shape) with `other`'s.

```cpp
auto x = pluss::make_series({1.0, 2.0});
auto other = pluss::make_series({7.0, 8.0, 9.0});

x.set_(other);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[7, 8, 9]
```

Example 2. `set_()` with no argument is a verified no-op.

```cpp
auto x = pluss::make_series({1.0, 2.0});

auto& ref = x.set_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 2]
same object: true
```

## Gotchas And Failure Modes

- `set_(other)` can change `self`'s shape, not just its values — it is not a same-shape value op like `add_`/`mul_`.
- The zero-argument `set_()` does nothing; don't confuse it with `set_(other)`.

## Summary

Use `set_(other)` to rebind `self` onto another `Series`'s storage wholesale. The zero-argument `set_()` is a no-op kept for API compatibility.

## See Also

- [`Series::copy_`](copy_.md): copies values into `self`'s *existing* storage instead of rebinding it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sensitivity.html">
    <span class="label">Previous</span>
    <span class="title">sensitivity</span>
  </a>
  <a class="next-link" href="setdiff1d.html">
    <span class="label">Next</span>
    <span class="title">setdiff1d</span>
  </a>
</div>
