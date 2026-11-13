<!-- pluss-hand-authored -->

# Series::cartprod

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::cartprod(const Series& other) const
Series Series::cartprod(pluss::RangeView<Series> others) const
```

## What It Does

`cartprod()` computes a cartesian product of value collections.

For two inputs, the result is a two-column table:

- column `0` comes from `self`
- column `1` comes from `other`

For more than two inputs, each extra input contributes one more column.

This method is one of the clearer examples of a root `Series` method that uses
shape as a convenient output format for a fundamentally set/list-like idea. It
still flattens each input first, but unlike `union1d()` or `intersect1d()`, it
does not collapse to unique values. It enumerates combinations.

## Overload Map

**`cartprod(other)`**

Build the pairwise cartesian product and return a 2D result with two columns.

**`cartprod(others)`**

Generalize to many inputs. Each extra input appends one column.

## Parameters

**other** (`const Series&`)

The second input for the two-input product.

**others** (`pluss::RangeView<Series>`)

The additional inputs for the many-input product.

In user code, brace lists such as `{b, c}` are the normal spelling.

All participating inputs must share:

- dtype
- platform

Each input is flattened before the product is formed.

## Returns

`Series`
    A 2D series where each row is one combination and each column corresponds
    to one input source.

For two inputs with sizes `n` and `m`, the output shape is `[n * m, 2]`.

## Gradient

`cartprod()` is not a differentiable numeric transform.

It is a combinatorial enumeration operation.

## Examples

Example 1. Two inputs produce a two-column matrix of all pairs.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1, 2}, pluss::ScalarType::Int64).cartprod(
        pluss::make_series({3, 4, 5}, pluss::ScalarType::Int64)));
```

### Output

```text
Series(shape=[6,2], strides=[2,1], dtype=int64, platform=cpu, numel=12, contiguous=true)
data=
⎡ 1 3 ⎤
⎢ 1 4 ⎥
⎢ 1 5 ⎥
⎢ 2 3 ⎥
⎢ 2 4 ⎥
⎣ 2 5 ⎦
```

Example 2. Passing several inputs appends one column per source.

```cpp
std::cout << pluss::make_series({1, 2}, pluss::ScalarType::Int64).cartprod({
    pluss::make_series({3, 4, 5}, pluss::ScalarType::Int64),
    pluss::make_series({9}, pluss::ScalarType::Int64),
});
```

### Output

```text
pluss::Series(shape=[6,3], strides=[3,1], dtype=int64, platform=cpu, numel=18, contiguous=true)
data=[[1, 3, 9],
  [1, 4, 9],
  [1, 5, 9],
  [2, 3, 9],
  [2, 4, 9],
  [2, 5, 9]]
```

Example 3. Higher-rank inputs are flattened before combinations are formed.

```cpp
pluss::PrintOptions full;
full.max_elements = 32;
full.edge_items = 32;

pluss::series::print(
    std::cout,
    pluss::make_series({
        {1, 2},
        {3, 4},
    }, pluss::ScalarType::Int64).cartprod(
        pluss::make_series({8, 9}, pluss::ScalarType::Int64).reshape({2, 1})),
    full);
```

### Output

```text
Series(shape=[8,2], strides=[2,1], dtype=int64, platform=cpu, numel=16, contiguous=true)
data=
⎡ 1 8 ⎤
⎢ 1 9 ⎥
⎢ 2 8 ⎥
⎢ 2 9 ⎥
⎢ 3 8 ⎥
⎢ 3 9 ⎥
⎢ 4 8 ⎥
⎣ 4 9 ⎦
```

Example 4. If any participating input is empty, the product is empty.

```cpp
std::cout << pluss::series::zeros({0}, pluss::ScalarType::Int64).cartprod(
    pluss::make_series({3, 4}, pluss::ScalarType::Int64));
```

### Output

```text
pluss::Series(shape=[0,2], strides=[2,1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

Example 5. Dtype mismatch is not allowed.

```cpp
try {
  std::cout << pluss::make_series({1, 2}, pluss::ScalarType::Int64).cartprod(
      pluss::make_series({1.0, 2.0})) << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
cartprod: input dtypes must match
```

## Gotchas And Failure Modes

- `cartprod()` does not deduplicate inputs for you.
- Inputs are flattened before the product is built.
- The number of rows grows multiplicatively. This method can become large very
  quickly.
- An empty input collapses the whole product to empty.
- Dtype mismatch and platform mismatch fail.

## Summary

Use `cartprod()` when you want explicit combinations, not a set summary.

- two inputs produce two columns
- many inputs produce one column per source
- flattening happens first
- duplicates stay duplicates
- output size multiplies across inputs, so this method can grow fast

## See Also

- [`Series::unionize`](unionize.md): combine membership without enumerating
  combinations.
- [`Series::pile`](pile.md): stack series structurally rather than
  combinatorially.
- [`Series::unite`](unite.md): concatenate along an existing dimension.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="calc.html">
    <span class="label">Previous</span>
    <span class="title">calc</span>
  </a>
  <a class="next-link" href="cauchy_.html">
    <span class="label">Next</span>
    <span class="title">cauchy_</span>
  </a>
</div>
