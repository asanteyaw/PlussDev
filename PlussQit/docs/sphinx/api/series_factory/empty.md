<!-- pluss-hand-authored -->

# pluss::series::empty

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::empty(sizes, ScalarType dt)
Series pluss::series::empty(sizes, Config cfg)
Series pluss::series::empty(sizes, opts...)
Series pluss::series::empty(sizes, {{"key", value}, ...})
```

## What It Builds

Allocates a `Series` with the requested shape and dtype, but does not promise a
meaningful initial numeric payload.

That is the key contract:

- `empty()` is about allocation
- `zeros()`, `ones()`, and `full()` are about allocation plus initialization

So `empty()` is the right tool when you intend to write every element
immediately afterward. It is the wrong tool when you need a known starting
value.

## Parameters

**sizes**

The requested shape. This can be passed through the usual shape forms accepted
through the factory surface.

**dt** (`ScalarType`)

The explicit dtype for the simple overload.

**cfg** (`Config`)

The configuration surface for dtype/platform routing.

**opts...` / key-value config**

Sugar that ultimately feeds the same config path. These forms matter for call
ergonomics, not for fundamentally different `empty()` behavior.

## Returns

`Series`
    A newly allocated series with the requested metadata, ready for immediate
    overwrite.

## Safe Usage Rule

Treat `empty()` storage as write-first.

In practice, that means a safe pattern looks like:

1. allocate with `empty()`
2. immediately fill or copy into it
3. only then read or print it

The examples below follow that rule on purpose.

## Examples

Example 1. Allocate scratch storage, then immediately initialize it before use.

```cpp
auto scratch = pluss::series::empty({2, 2}, pluss::ScalarType::Float64);
scratch.copy_(pluss::series::make({
    {5.0, 6.0},
    {7.0, 8.0},
}));

std::cout << scratch;
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[5, 6],
  [7, 8]]
```

The important part is the order: allocate first, read later.

Example 2. `empty()` is equally valid for non-floating dtypes.

```cpp
auto ids = pluss::series::empty({3}, pluss::ScalarType::Int32);
ids.copy_(pluss::series::make({10, 20, 30}, pluss::ScalarType::Int32));

std::cout << ids;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=int32, platform=cpu, numel=3, contiguous=true)
data=[10, 20, 30]
```

Example 3. Scalar scratch buffers follow the same write-first rule.

```cpp
auto scratch_scalar = pluss::series::empty({}, pluss::ScalarType::Float64);
scratch_scalar.copy_(pluss::series::make(3.25));

std::cout << scratch_scalar;
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=3.25
```

Example 4. The config path changes metadata policy, not the "allocate only"
contract.

```cpp
auto mask = pluss::series::empty({2}, pluss::Config{}.with_dtype(pluss::ScalarType::Bool));
mask.copy_(pluss::series::make({true, false}, pluss::ScalarType::Bool));

std::cout << mask;
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[true, false]
```

## Notes

- `empty()` allocates metadata and storage, not meaningful values.
- Prefer `zeros()`, `ones()`, or `full()` when you need predictable initial
  contents.
- The config and option-tag overloads change routing/dtype policy, not the core
  "uninitialized payload" contract.

## See Also

- [`pluss::series::zeros`](zeros.md): same shape-first idea, but initialized to
  zero.
- [`pluss::series::full`](full.md): initialized to a caller-chosen fill value.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="arange.html">
    <span class="label">Previous</span>
    <span class="title">arange</span>
  </a>
  <a class="next-link" href="full.html">
    <span class="label">Next</span>
    <span class="title">full</span>
  </a>
</div>
