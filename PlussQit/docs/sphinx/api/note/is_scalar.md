# Note::is_scalar

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
bool Note::is_scalar() const noexcept
```

## Test For Rank Zero

`is_scalar()` is true only for a defined rank-zero Note. It describes geometry,
not cardinality: a vector with shape `[1]` contains one cell but is not scalar.
That distinction determines whether indexing requires zero or one coordinates
and whether shape-preserving operations return `[]` or `[1]`.

An undefined handle reports false even though its non-throwing `dim()` fallback
is zero. Empty text does not affect the result. The query is constant-time,
allocation-free, and `noexcept`.

## Example 1: Scalar Versus Singleton Vector

```cpp
const auto scalar = pluss::note::make("alpha");
const auto singleton = pluss::note::make({"alpha"});
std::cout << std::boolalpha
          << "scalar: " << scalar.is_scalar() << " sizes=" << scalar.sizes().size() << '\n'
          << "singleton: " << singleton.is_scalar() << " dim=" << singleton.dim() << '\n';
```

### Output

```text
scalar: true sizes=0
singleton: false dim=1
```

## Example 2: Empty Text Remains Scalar

```cpp
const auto value = pluss::note::make("");
std::cout << std::boolalpha
          << "scalar=" << value.is_scalar()
          << " empty=" << value.empty()
          << " bytes=" << value.lengths_bytes()[0] << '\n';
```

### Output

```text
scalar=true empty=false bytes=0
```

## Example 3: Undefined Is Not Scalar

```cpp
const auto value = pluss::Note::undefined();
std::cout << std::boolalpha
          << "defined=" << value.defined()
          << " dim=" << value.dim()
          << " scalar=" << value.is_scalar() << '\n';
```

### Output

```text
defined=false dim=0 scalar=false
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_matrix.html"><span class="label">Previous</span><span class="title">is_matrix</span></a>
  <a class="next-link" href="is_tensor3.html"><span class="label">Next</span><span class="title">is_tensor3</span></a>
</div>
<!-- pluss-hand-authored -->
