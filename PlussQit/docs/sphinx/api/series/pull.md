<!-- pluss-hand-authored -->

# Series::pull

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
template<class T>
std::vector<T> Series::pull() const
```

## Crossing A One-Dimensional Series Into C++

`pull<T>()` materializes a rank-one `Series` as an owning
`std::vector<T>`. It is the direct companion to [`item<T>()`](item.md):
`item()` crosses the host boundary for shape `[]`, while `pull()` crosses it
for shape `[n]`.

The rank check is intentional. A matrix is not silently flattened, and a
scalar is not silently wrapped in a one-element vector. Generic code that must
accept arbitrary rank should use [`host<T>()`](host.md), where shape and stride
metadata remain available.

`T` selects the C++ element representation. The Series is converted while it
is materialized, so a floating Series may be pulled as `double`, an integer
Series as `std::int64_t`, and a Boolean Series as `bool`. Conversion follows
the same dtype rules as `Series::to(...)`.

## Ownership, Placement, And Cost

The returned vector owns its allocation. Changing it cannot mutate the Series,
and later Series mutations cannot change the vector. A non-CPU receiver is
synchronized and copied to host memory before conversion. Consequently,
`pull()` belongs at an interoperability, logging, serialization, or testing
boundary—not inside a per-lane numerical loop.

`pull()` is not differentiable because `std::vector<T>` has no Series graph
identity. The receiver and its graph are unchanged; only the extracted C++
copy lies outside reverse mode.

## Examples

### 1. Pull a floating vector

```cpp
const auto values = pluss::make_series({1.25, 2.5, 5.0});
const auto host = values.pull<double>();

for (double value : host) std::cout << value << ' ';
std::cout << '\n';
```

#### Output

```text
1.25 2.5 5 
```

The `auto` variable is an ordinary `std::vector<double>` because `pull()` has
one fixed native return type.

### 2. Convert the element representation while pulling

```cpp
const auto values = pluss::make_series({2, 4, 8});
const std::vector<double> host = values.pull<double>();

std::cout << host.size() << ' ' << host.front() << ' ' << host.back() << '\n';
```

#### Output

```text
3 2 8
```

The Series remains integral. Only the owning host copy uses `double`.

### 3. Preserve an empty one-dimensional shape

```cpp
const auto empty = pluss::series::empty({0}, pluss::ScalarType::Float64);
const auto host = empty.pull<double>();

std::cout << "dim=" << empty.dim()
          << " size=" << host.size() << '\n';
```

#### Output

```text
dim=1 size=0
```

An empty vector is still rank one, so it satisfies the contract.

### 4. Pull a Boolean mask

```cpp
const auto mask = pluss::make_series({true, false, true});
const auto host = mask.pull<bool>();

std::cout << std::boolalpha
          << host[0] << ' ' << host[1] << ' ' << host[2] << '\n';
```

#### Output

```text
true false true
```

The result is `std::vector<bool>`, including its standard packed Boolean
storage behavior.

### 5. Reject scalar and matrix receivers

```cpp
for (const auto& value : {
         pluss::make_series(7),
         pluss::make_series({1, 2, 3, 4}, {2, 2})}) {
  try {
    (void)value.pull<std::int64_t>();
  } catch (const std::exception& error) {
    std::cout << error.what() << '\n';
  }
}
```

#### Output

```text
Series::pull<T>(): expected a rank-one Series
Series::pull<T>(): expected a rank-one Series
```

Use `item<T>()` for the scalar. Use `host<T>()` for the matrix instead of
discarding its two-dimensional structure.

## See Also

- [`item()`](item.md): rank-zero native extraction.
- [`host()`](host.md): rank-aware extraction for arbitrary dimensions.
- [`dim()`](dim.md): query the rank checked by `pull()`.
- [`to()`](to.md): remain inside Series while changing dtype or platform.
