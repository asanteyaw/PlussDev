# TimeIndex::normalize

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signature

```cpp
TimeIndex::normalize() const -> TimeIndex
```

## What It Does

Normalizes each timestamp to midnight of its calendar day.

This is effectively the same as `floor("D")`, but the dedicated name reads
better when your intent is explicitly "strip intraday time."

Use it when the day identity matters and the intraday clock should be discarded.

## Returns

`TimeIndex`
    The normalized daily axis.

## Examples

Example 1. Remove the clock component and keep only the calendar day.

```cpp
auto out = base.normalize();
std::cout << pluss::print_timeline(out) << "\n";
```

### Output

```text
Timeline(shape=[5], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 00:00:00, 2024-11-01 00:00:00, 2024-11-01 00:00:00, 2024-11-01 00:00:00 ]
```

Example 2. `normalize()` is the clearer call when the intent is explicitly
"date-only axis semantics."

```cpp
auto days = base.normalize();
std::cout << pluss::print_timeline(days) << "\n";
```

## See Also

- [`TimeIndex::floor`](floor.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="round.html">
    <span class="label">Previous</span>
    <span class="title">round</span>
  </a>
  <a class="next-link" href="to_note_iso.html">
    <span class="label">Next</span>
    <span class="title">to_note_iso</span>
  </a>
</div>
