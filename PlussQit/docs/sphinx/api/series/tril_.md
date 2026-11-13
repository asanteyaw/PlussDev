<!-- pluss-hand-authored -->

# Series::tril_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::tril_() const
Series& Series::tril_(std::int64_t diagonal) const
```

## What It Does

`tril_()` is the in-place sibling of [`tril()`](tril.md).

It applies the same lower-triangular masking rule, but instead of returning a
fresh value it mutates the receiver and returns the same handle by reference.

That difference matters in three places:

- later reads from the same object see the zeroed result
- aliasing handles to the same storage see the same mutation
- mutation-sensitive state such as [`storage_version()`](storage_version.md)
  changes

## Overload Map

**`tril_()`**

Equivalent to `tril_(0)`.

**`tril_(std::int64_t diagonal)`**

Mutating lower-triangular masking with an explicit keep boundary.

## Parameters

**diagonal** (`std::int64_t`)

The highest diagonal that remains non-zero after the mutation.

## Returns

`Series&`
    The same `Series`, after in-place mutation.

## Examples

Example 1. `tril_()` mutates the receiver and bumps its storage version.

```cpp
auto square = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9},
}, pluss::ScalarType::Int64);

std::cout << "before=" << square.storage_version() << "\n";
square.tril_(1);
std::cout << "after=" << square.storage_version() << "\n";
std::cout << square;
```

### Output

```text
before=0
after=1
pluss::Series(shape=[3,3], strides=[3,1], dtype=int64, platform=cpu, numel=9, contiguous=true)
data=[[1, 2, 0],
  [4, 5, 6],
  [7, 8, 9]]
```

## Gotchas And Failure Modes

- `tril_()` mutates. Use [`tril()`](tril.md) if you need value semantics.
- The zero-argument form is just the main-diagonal case.
- Storage-version-sensitive code can use `storage_version()` to detect the
  mutation.

## Summary

Use `tril_()` when the lower-triangular mask should become the new state of the
current `Series`, not just a derived copy.

## See Also

- [`Series::tril`](tril.md): copy-returning lower-triangular masking.
- [`Series::triu_`](triu_.md): in-place upper-triangular companion.
- [`Series::storage_version`](storage_version.md): mutation detection surface.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tril.html">
    <span class="label">Previous</span>
    <span class="title">tril</span>
  </a>
  <a class="next-link" href="triu.html">
    <span class="label">Next</span>
    <span class="title">triu</span>
  </a>
</div>
