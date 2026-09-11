# Note::is_vector

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
bool Note::is_vector() const noexcept
```

## Test For One-Dimensional Geometry

`is_vector()` is true exactly when a defined Note has one axis. Its length may
be zero, one, or larger: vector-ness is a rank property, not a statement about
cardinality. This is the appropriate guard before algorithms whose contract is
a single ordered text sequence.

The method does not flatten matrices and does not classify a scalar as a
one-element vector. Undefined handles report false. The query is constant-time,
allocation-free, and `noexcept`.

## Example 1: One Cell Can Have Two Different Ranks

```cpp
const auto scalar = pluss::note::make("x");
const auto vector = pluss::note::make({"x"});
std::cout << std::boolalpha
          << "scalar vector=" << scalar.is_vector() << '\n'
          << "[1] vector=" << vector.is_vector() << '\n';
```

### Output

```text
scalar vector=false
[1] vector=true
```

## Example 2: An Empty Vector Is Still A Vector

```cpp
const auto value = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 1>{0});
std::cout << std::boolalpha
          << "vector=" << value.is_vector()
          << " empty=" << value.empty()
          << " size=" << value.size(0) << '\n';
```

### Output

```text
vector=true empty=true size=0
```

## Example 3: Higher Ranks Do Not Qualify

```cpp
const auto matrix = pluss::note::make({{"x"}});
const auto absent = pluss::Note::undefined();
std::cout << std::boolalpha
          << "matrix=" << matrix.is_vector()
          << " absent=" << absent.is_vector() << '\n';
```

### Output

```text
matrix=false absent=false
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_utf8_valid.html"><span class="label">Previous</span><span class="title">is_utf8_valid</span></a>
  <a class="next-link" href="is_whitespace.html"><span class="label">Next</span><span class="title">is_whitespace</span></a>
</div>
<!-- pluss-hand-authored -->
