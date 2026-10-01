<!-- pluss-hand-authored -->

# Series::platform

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Platform Series::platform() const noexcept
```

## Ask Where The Series Executes

`platform()` returns the backend that owns the Series implementation. It is a
metadata query: it does not inspect elements, synchronize a device, allocate a
buffer, or move data. CPU, MPS, and CUDA values can therefore be dispatched
without first crossing a host boundary.

Use the result when code genuinely has backend-specific policy, such as
choosing a supported precision or reporting placement. Do not branch merely to
perform ordinary Series arithmetic; supported operations already dispatch from
this metadata. To transfer a value, call [`to(Platform)`](to.md). There is no
second transfer meaning hidden behind `platform()`.

## Return And Lifetime Contract

The return is the small `Platform` enum, exposed publicly as `pluss::pCPU`,
`pluss::pMPS`, and `pluss::pCUDA`. Views, reshapes, and ordinary operations
preserve platform unless their documented purpose is transfer. Copies also
remain on the same platform. Because this query is `noexcept` and performs no
work proportional to `numel()`, it is suitable for logging and dispatch paths.

## Examples

### 1. Query A Newly Constructed Series

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
std::cout << x.platform();
```

### Output

```text
cpu
```

### 2. Views And Copies Preserve Placement

```cpp
auto table = pluss::make_series({
    {4.0, 1.0, 4.0, 2.0},
    {3.0, 5.0, 0.0, 5.0},
});
std::cout << "source=" << table.platform() << "\n"
          << "view=" << table.transpose(0, 1).platform() << "\n"
          << "copy=" << table.clone().platform();
```

### Output

```text
source=cpu
view=cpu
copy=cpu
```

### 3. Use The Enum In Native C++ Dispatch

```cpp
if (table.platform() == pluss::pCPU) {
  std::cout << "host path";
} else {
  std::cout << "device path";
}
```

### Output

```text
host path
```

An undefined Series has no backing implementation; avoid using metadata from
undefined values as application state. `platform()` is non-differentiable
because it returns host metadata rather than a Series.

## See Also

- [`Series::to`](to.md): transfer platform and/or convert dtype.
- [`Series::dtype`](dtype.md): query element representation.
- [`Series::strides`](strides.md): query storage traversal metadata.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pile.html"><span class="label">Previous</span><span class="title">pile</span></a>
  <a class="next-link" href="polygamma.html"><span class="label">Next</span><span class="title">polygamma</span></a>
</div>
