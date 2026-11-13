# Timeline::fill_nat

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::fill_nat(const Timeline& value)
```

## What It Does

Replaces absent instants with one scalar replacement instant.

This is the narrow, direct substitution surface for absent instants.

## Parameters

**value**

The replacement timestamp.

Current verified behavior requires this to be a scalar `Timeline`, not another
vector of replacements.

## Returns

`Timeline`
    A Timeline with absent instants replaced.

## Examples

Example 1. Replace every absent instant with one explicit fallback instant.

```cpp
auto out = with_nat.fill_nat(pluss::timeline::make("2024-11-15T00:00:00Z"));
std::cout << pluss::timeline::print(out) << "\n";
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2024-11-01 09:00:00, 2024-11-15 00:00:00, 2024-12-31 23:59:59, 2024-11-15 00:00:00 ]
```

## Notes

- `fill_nat()` is for one scalar fill value.
- Use [`where`](where.md) when the replacement depends on a more general mask
  or another shaped timeline.

## See Also

- [`Timeline::isnat`](isnat.md)
- [`Timeline::where`](where.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fill_like.html">
    <span class="label">Previous</span>
    <span class="title">fill_like</span>
  </a>
  <a class="next-link" href="where.html">
    <span class="label">Next</span>
    <span class="title">where</span>
  </a>
</div>
