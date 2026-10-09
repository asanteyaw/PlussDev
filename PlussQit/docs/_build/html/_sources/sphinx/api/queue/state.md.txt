<!-- pluss-hand-authored -->

# Queue::defined, Queue::dim, Queue::numel

[Back to Queue API Reference](../../queue_api_reference)

## Signature

```cpp
bool defined() const noexcept;
std::int64_t dim() const;
std::int64_t numel() const;
```

## What It Does

These three forward directly to the held payload's own `defined()`,
`dim()`, and `numel()` -- there is no `Queue`-specific shape or definedness
concept layered on top. They work the same way regardless of which of the
four kinds the `Queue` holds, which is the point: generic code can read a
`Queue`'s shape without first checking `kind()`.

## Parameters

None.

## Returns

`defined()` returns `bool`. `dim()` and `numel()` return `std::int64_t`,
matching the held payload's own rank and element count.

## Examples

Example 1. A 1D `Series`-holding `Queue`.

```cpp
pluss::Queue q(pluss::make_series({1.0, 2.0, 3.0, 4.0}));
std::cout << "defined(): " << q.defined() << '\n';
std::cout << "dim(): " << q.dim() << '\n';
std::cout << "numel(): " << q.numel() << '\n';
```

### Output

```text
defined(): true
dim(): 1
numel(): 4
```

Example 2. A scalar `Duration`-holding `Queue` -- `dim()` is `0`, not `1`,
and `numel()` is `1` for a single scalar value, the same convention every
PlussQit payload type uses for a 0D value.

```cpp
pluss::Queue scalar_duration(pluss::duration::days(1));
std::cout << "duration dim(): " << scalar_duration.dim() << '\n';
std::cout << "duration numel(): " << scalar_duration.numel() << '\n';
```

### Output

```text
duration dim(): 0
duration numel(): 1
```

## See Also

- [`kind`](kind.md): identifying the payload these three describe.
- [`accessors`](accessors.md): getting the full payload back, not just its
  shape.
