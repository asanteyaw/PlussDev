<!-- pluss-hand-authored -->

# Series::host

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
template<class T>
Series::HostValue<T> Series::host() const

Series::HostAny Series::host() const
```

## The Rank-Aware Host Boundary

`host()` materializes an owning CPU representation without pretending that
every Series is a flat vector. The typed overload converts every lane to `T`
and returns `HostValue<T>`. The untyped overload preserves the runtime
`ScalarType` in `HostAny`.

The selected payload follows rank:

| Series rank | Typed state | Untyped state | Native meaning |
|---:|---|---|---|
| `0` | `is_scalar()` | `is_scalar()` | one scalar value |
| `1` | `is_vector()` | `is_vector()` | one `std::vector` |
| `2+` | `is_array()` | `is_array()` | flat owning data plus sizes, strides, and contiguity |

Rank—not `numel()`—selects the state. A shape `[1]` Series remains a vector,
whereas shape `[]` is scalar. This distinction is why `host()` is appropriate
for generic boundaries and why [`item()`](item.md) and [`items()`](items.md) can
enforce narrower contracts.

For an N-D result, `array().data` is contiguous row-major logical data.
`array().sizes` records the original shape; `array().strides` records the
source layout; and `array().contiguous` reports whether the source was already
contiguous before materialization. Thus a transposed view exports values in
logical order without falsely reporting that its source layout was dense.

## Ownership, Conversion, And Autodiff

Every payload owns its host allocation. There is no alias back into Series
storage. Device-backed values synchronize and transfer once; strided values
are traversed in logical row-major order; and typed export performs the
requested element conversion.

Host extraction is an observation boundary, not a differentiable operation.
It neither mutates nor detaches the Series, but calculations performed on the
returned C++ storage cannot contribute to `reverse()`.

## Examples

### 1. Distinguish a scalar from a singleton vector

```cpp
const auto scalar = pluss::make_series(7).host<std::int64_t>();
const auto vector = pluss::make_series({7}).host<std::int64_t>();

std::cout << std::boolalpha
          << "scalar=" << scalar.is_scalar()
          << " vector=" << vector.is_vector() << '\n';
```

#### Output

```text
scalar=true vector=true
```

Both payloads contain one value, but their ranks remain different.

### 2. Inspect a matrix without losing its geometry

```cpp
const auto host =
    pluss::make_series({1, 2, 3, 4, 5, 6}, {2, 3})
        .host<std::int64_t>();
const auto& array = host.array();

std::cout << "shape=" << array.sizes[0] << 'x' << array.sizes[1]
          << " flat=";
for (auto value : array.data) std::cout << value << ' ';
std::cout << '\n';
```

#### Output

```text
shape=2x3 flat=1 2 3 4 5 6 
```

The flat allocation is accompanied by enough metadata to reconstruct the
matrix exactly.

### 3. Preserve source-layout information while materializing logical order

```cpp
const auto view =
    pluss::make_series({1, 2, 3, 4, 5, 6}, {2, 3}).transpose(0, 1);
const auto host = view.host<std::int64_t>();

std::cout << std::boolalpha
          << "shape=" << host.array().sizes[0] << 'x'
          << host.array().sizes[1]
          << " source_contiguous=" << host.array().contiguous
          << " values=";
for (auto value : host.array().data) std::cout << value << ' ';
std::cout << '\n';
```

#### Output

```text
shape=3x2 source_contiguous=false values=1 4 2 5 3 6 
```

The exported data is contiguous and logically ordered; the flag describes the
receiver from which it was produced.

### 4. Request an explicit host element type

```cpp
const auto host = pluss::make_series({1, 2, 3}).host<double>();
std::cout << host.array().data[0] << ' '
          << host.array().data[1] << ' '
          << host.array().data[2] << '\n';
```

#### Output

```text
1 2 3
```

The Series dtype is unchanged; the host payload contains `double` values.
`host()` does not expose a second vector-extraction method: use `items<double>()`
when rank one is part of the contract, and inspect `array().data` when generic
code is already handling arbitrary rank and metadata.

### 5. Preserve runtime dtype with untyped export

```cpp
const auto host = pluss::make_series({true, false}).host();
const auto& values = host.vector_as<bool>();

std::cout << host.dtype() << ' ' << std::boolalpha
          << values[0] << ' ' << values[1] << '\n';
```

#### Output

```text
bool true false
```

Use untyped export when generic code must inspect the runtime dtype rather than
choosing a conversion type at the call site.

## See Also

- [`item()`](item.md): require rank zero and return one native scalar.
- [`items()`](items.md): require rank one and return `std::vector<T>` directly.
- [`sizes()`](sizes.md): inspect shape without materializing values.
- [`strides()`](strides.md): inspect source layout without a host transfer.
- [`contiguous()`](contiguous.md): produce a dense Series while remaining in
  Series space.
