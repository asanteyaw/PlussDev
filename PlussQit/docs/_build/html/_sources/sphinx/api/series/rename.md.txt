<!-- pluss-hand-authored -->

# Series::rename

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::rename_() const
```

## What It Does

`rename_()` is a torch-parity stub: Pluss's `Series` does not carry named dimensions, so there is nothing to rename. It is a pure no-op — it returns a reference to `self` unmodified, without touching its storage or bumping its version.

It exists on the public surface purely so code written against a torch-style API (`x.rename_(...)`) still compiles and does something well-defined, rather than failing to resolve.

## Parameters

None on this surface.

## Returns

`Series&` — a reference to `self`, unmodified.

## Examples

Example 1. `rename_()` returns `self` unchanged — genuinely the same object, same values.

```cpp
auto x = pluss::make_series({1.0, 2.0});

auto& ref = x.rename_();
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

- This is a stub, not a real named-dimension feature — don't rely on it to attach any metadata.

## Summary

`rename_()` is a no-op kept for torch-style API compatibility; `Series` has no named-dimension concept to rename.

## See Also

- [`Series::squeeze`](squeeze.md), [`Series::unsqueeze`](unsqueeze.md): real shape-affecting structural ops.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remainder.html">
    <span class="label">Previous</span>
    <span class="title">remainder</span>
  </a>
  <a class="next-link" href="renorm.html">
    <span class="label">Next</span>
    <span class="title">renorm</span>
  </a>
</div>
