<!-- pluss-hand-authored -->

# Duration::defined

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
bool Duration::defined() const noexcept
```

## What It Tells You

`defined()` reports whether the handle owns duration storage. It answers a
storage-state question, not whether the duration is nonzero, nonempty, or free
of `NaT` values.

Public Duration factories always return defined objects. A scalar zero, a
shape containing a zero-length axis, and an array whose lanes are all `NaT`
are therefore all defined. Use [`numel()`](numel.md) for cardinality and
[`isnat()`](isnat.md) for lane-wise absence.

Duration currently has no public undefined factory and cannot be default
constructed. The query exists because library internals and owning wrappers
still need a cheap, non-throwing way to inspect handle state.

## Returns

`true` when backing storage exists; otherwise `false`. The check is constant
time, performs no allocation, and does not inspect individual lanes.

## Examples

Example 1. A scalar produced by a unit factory is defined.

```cpp
const auto value = pluss::duration::seconds(90);
std::cout << std::boolalpha << "defined=" << value.defined()
          << " dim=" << value.dim()
          << " numel=" << value.numel() << "\n";
```

### Output

```text
defined=true dim=0 numel=1
```

Example 2. A zero-element array is still a real, defined object.

```cpp
const auto empty = pluss::duration::zeros({2, 0, 3});
std::cout << std::boolalpha << "defined=" << empty.defined()
          << " dim=" << empty.dim()
          << " numel=" << empty.numel() << "\n";
```

### Output

```text
defined=true dim=3 numel=0
```

Example 3. `NaT` is lane data, not an undefined handle.

```cpp
const auto value = pluss::duration::nanos(pluss::Duration::kNaT);
std::cout << std::boolalpha
          << "defined=" << value.defined()
          << " isnat=" << value.isnat().item<bool>() << "\n";
```

### Output

```text
defined=true isnat=true
```

## See Also

- [`numel`](numel.md): distinguish allocated emptiness from scalar storage.
- [`isnat`](isnat.md): test missing lanes rather than handle state.
- [`sizes`](sizes.md): inspect the complete logical shape.
