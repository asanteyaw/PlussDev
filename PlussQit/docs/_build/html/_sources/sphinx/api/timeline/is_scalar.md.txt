<!-- pluss-hand-authored -->

# Timeline::is_scalar

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
bool Timeline::is_scalar() const
```

## What It Tells You

`is_scalar()` is the readable form of `dim() == 0`. It answers whether the
timeline represents one unindexed instant rather than an array with axes.

This distinction is important for scalar-only operations such as
[`isoformat()`](isoformat.md), scalar bounds passed to range builders, and APIs
that require exactly one instant rather than merely one stored position.

A shape `[1]` timeline also contains one position, but it is a vector and
therefore returns `false`. Cardinality and scalar structure must not be used
interchangeably.

## Returns

`true` exactly when the logical rank is zero.

## Examples

Example 1. Contrast a scalar with a one-element vector.

```cpp
auto scalar = pluss::timeline::make("2024-02-29T12:30:00Z");
auto singleton = pluss::timeline::make({"2024-02-29T12:30:00Z"});

std::cout << std::boolalpha
          << "scalar: numel=" << scalar.numel()
          << ", is_scalar=" << scalar.is_scalar() << "\n"
          << "singleton: numel=" << singleton.numel()
          << ", is_scalar=" << singleton.is_scalar() << "\n";
```

### Output

```text
scalar: numel=1, is_scalar=true
singleton: numel=1, is_scalar=false
```

Example 2. Guard a scalar-only formatter at an API boundary.

```cpp
auto render_one = [](const pluss::Timeline& value) {
    if (!value.is_scalar()) {
        return std::string{"expected one scalar instant"};
    }
    return value.isoformat();
};

auto scalar = pluss::timeline::make("2024-02-29T12:30:00Z");
auto vector = pluss::timeline::make({"2024-02-29T12:30:00Z"});
std::cout << render_one(scalar) << "\n" << render_one(vector) << "\n";
```

### Output

```text
2024-02-29T12:30:00.000000000+00:00
expected one scalar instant
```

Example 3. `host()` has a deliberately different decision rule: it condenses
any one-position timeline to its scalar host payload.

```cpp
auto singleton = pluss::timeline::make({"2024-02-29T12:30:00Z"});
auto host = singleton.host();
std::cout << std::boolalpha
          << "dim=" << singleton.dim()
          << ", is_scalar=" << singleton.is_scalar()
          << ", host scalar=" << host.is_scalar() << "\n";
```

### Output

```text
dim=1, is_scalar=false, host scalar=true
```

## Gotchas And Failure Modes

- Do not replace this test with `numel() == 1` when an API requires rank zero.
- `is_scalar()` says nothing about validity of the timestamp; a scalar may
  hold `NaT`.
- `host().is_scalar()` describes the selected host payload variant, not the
  original timeline's rank.

## See Also

- [`Timeline::dim`](dim.md)
- [`Timeline::numel`](numel.md)
- [`Timeline::host`](host.md)
- [`Timeline::isoformat`](isoformat.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="numel.html"><span class="label">Previous</span><span class="title">numel</span></a>
  <a class="next-link" href="tz.html"><span class="label">Next</span><span class="title">tz</span></a>
</div>
