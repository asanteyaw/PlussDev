# Timeline::isnat_scalar

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline::isnat_scalar() -> bool
```

## What It Does

Returns whether a scalar `Timeline` is missing.

This is the scalar-only companion to [`isnat`](isnat.md):

- `isnat()`
  Returns a boolean `Series` for array-shaped timelines.
- `isnat_scalar()`
  Returns one `bool` for a 0D timeline.

## Returns

`bool`
    `true` when the scalar timestamp is missing, otherwise `false`.

## Examples

Example 1. A defined scalar timeline is not missing.

```cpp
auto flag = scalar.isnat_scalar();
std::cout << (flag ? "true" : "false") << "\n";
```

### Output

```text
false
```

Example 2. The documented scalar missing-time path reports `true`.

```cpp
auto flag = scalar_nat.isnat_scalar();
std::cout << (flag ? "true" : "false") << "\n";
```

### Output

```text
true
```

## Notes

- This method expects a scalar timeline. It is not the right surface for 1D,
  2D, or 3D temporal payloads.
- The current text-input path for a scalar missing timestamp is the empty
  string, not the literal text `"NaT"`.

## See Also

- [`Timeline::isnat`](isnat.md)
- [`Timeline::isoformat`](isoformat.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isvalid.html">
    <span class="label">Previous</span>
    <span class="title">isvalid</span>
  </a>
  <a class="next-link" href="argsort.html">
    <span class="label">Next</span>
    <span class="title">argsort</span>
  </a>
</div>
