<!-- pluss-hand-authored -->

# Series::scatter_reduce_sum

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::scatter_reduce_sum(
    std::int64_t dim,
    const Series& index,
    const Series& src,
    bool include_self) const

Series Series::scatter_reduce_sum(
    std::int64_t dim,
    const Series& index,
    const Series& src,
    bool include_self,
    double initial_scalar) const
```

## What It Does

`scatter_reduce_sum()` is the typed additive wrapper around
[`scatter_reduce()`](scatter_reduce.md).

It is the most direct way to say:

```text
Reduce repeated indexed writes by addition.
```

Compared with the general form, it removes the reduction string. Compared with
[`scatter_add()`](scatter_add.md), it exposes the extra knobs that matter when
you want to exclude the old destination value or seed every output lane with an
explicit initial value.

## Parameters

**dim**, **index**, and **src**

These match the general `scatter_reduce()` contract.

**include_self** (`bool`)

Whether each old destination value in `self` is part of the sum.

**initial_scalar** (`double`)

An explicit seed added to every destination lane before the scattered values
reduce into it.

## Returns

`Series`
    A new `Series` containing the indexed additive reduction.

## Gradient

For a scalar objective, addition is linear:

- every participating base term receives the upstream sensitivity
- every participating source term receives the upstream sensitivity of the
  destination lane it contributed to

If `include_self = false`, targeted base lanes are removed from that path.

## Examples

Example 1. With `include_self = true`, the old destination value remains part
of the sum.

```cpp
auto base = pluss::make_series({10.0, 20.0, 30.0, 40.0});
auto index = pluss::make_series({1LL, 1LL});
auto src = pluss::make_series({1.0, 3.0});

std::cout << base.scatter_reduce_sum(0, index, src, true);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, 24, 30, 40]
```

Example 2. With `include_self = false`, only the scattered values are added.

```cpp
auto seeded = pluss::make_series({10.0, 20.0, 30.0, 40.0});

std::cout << seeded.scatter_reduce_sum(0, index, src, false, 5.0);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[5, 9, 5, 5]
```

Every destination starts from `5`; destination `1` then receives `1 + 3`.

Example 3. The wrapper is the typed equivalent of the general sum form.

```cpp
auto general = base.scatter_reduce(0, index, src, "sum", true);
auto typed = base.scatter_reduce_sum(0, index, src, true);

std::cout << "general=" << general << "\n";
std::cout << "typed=" << typed << "\n";
```

### Output

```text
general=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, 24, 30, 40]
typed=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, 24, 30, 40]
```

The result is identical; the difference is only how the call site expresses
intent.

## Gotchas And Failure Modes

- `include_self` still matters even though the reduction is fixed to sum.
- `initial_scalar` seeds every output lane, including untouched ones.
- If additive accumulation with `include_self = true` is all you need,
  [`scatter_add()`](scatter_add.md) is the shorter spelling.

## Summary

Use `scatter_reduce_sum()` when the reduction is definitely addition and you
want a typed wrapper with `include_self` and optional explicit seeding.

## See Also

- [`Series::scatter_add`](scatter_add.md): additive shorthand.
- [`Series::scatter_reduce`](scatter_reduce.md): general form.
- [`Series::scatter_reduce_amax`](scatter_reduce_amax.md)
- [`Series::scatter_reduce_amin`](scatter_reduce_amin.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scatter_reduce_amin.html">
    <span class="label">Previous</span>
    <span class="title">scatter_reduce_amin</span>
  </a>
  <a class="next-link" href="select.html">
    <span class="label">Next</span>
    <span class="title">select</span>
  </a>
</div>
