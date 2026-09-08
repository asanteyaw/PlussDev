# Timeline::isvalid

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline::isvalid() -> Series
```

Returns a boolean mask that marks the timestamps that are actually defined.

`isvalid()` is the positive-form validity check. Use it when the next operation
should receive represented instants rather than `NaT` positions.
That usually reads better than writing the same intent as `!t.isnat()`.

The returned `Series` follows the exact shape of the source `Timeline`:

- scalar timeline in, scalar boolean out
- 1D timeline in, 1D boolean mask out
- higher-rank timeline in, same higher-rank boolean structure out

## Parameters

This method takes no parameters.

The main behavioral choice is conceptual:

- use `isvalid()` when the positive case is the one you care about
- use [`isnat()`](isnat.md) when the absent case is the one you care about

## Returns

`Series`
    A boolean mask whose `true` positions correspond to defined timestamps and
    whose `false` positions correspond to `NaT`.

## Notes

- `isvalid()` is the complement of [`isnat()`](isnat.md).
- It is especially useful ahead of [`where(...)`](where.md), filtering, or
  replacement logic where the "good rows" path is the default branch.
- The method does not alter timezone information or timestamp values; it only
  inspects presence versus missingness.

## Examples

Example 1. Build a validity mask from a Timeline containing absent instants.

```cpp
auto keep = with_nat.isvalid();
std::cout << pluss::series::print(keep) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, false, true, false]
```

Example 2. Run the same check on a timeline where every timestamp is already
defined.

```cpp
auto keep = base.isvalid();
std::cout << pluss::series::print(keep) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, true, true, true]
```

Example 3. Use `isvalid()` when you want the positive branch to be explicit.

```cpp
auto repaired = with_nat.where(with_nat.isvalid(), with_nat.floor("D"));
std::cout << pluss::timeline::print(repaired) << "\n";
```

### Output

This call keeps the original timestamp where the mask is `true` and falls back
to the alternate timeline where the mask is `false`. See the verified
`where(...)` behavior on the canonical page:

```text
Timeline(shape=[4], tz=UTC)
[ 2024-11-01 00:00:00, NaT, 2024-12-31 00:00:00, NaT ]
```

## See Also

- [`Timeline::isnat`](isnat.md)
- [`Timeline::isnat_scalar`](isnat_scalar.md)
- [`Timeline::where`](where.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnat.html">
    <span class="label">Previous</span>
    <span class="title">isnat</span>
  </a>
  <a class="next-link" href="isnat_scalar.html">
    <span class="label">Next</span>
    <span class="title">isnat_scalar</span>
  </a>
</div>
